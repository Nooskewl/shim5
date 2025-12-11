#include "shim5/audio.h"
#include "shim5/flac.h"
#include "shim5/json.h"
#include "shim5/sample.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/vorbis.h"
#include "libutil/libutil.h"

#include "shim5/internal/audio.h"

using namespace noo;

namespace noo {

namespace audio {

void Sample::stop_instance(Sample_Instance *s)
{
	audio::lock_mutex();

	for (std::vector<Sample_Instance *>::iterator it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
		Sample_Instance *s2 = *it;
		if (s2 == s) {
			internal::audio_context.playing_samples.erase(it);
			delete s;
			break;
		}
	}

	audio::unlock_mutex();
}

Sample::Sample(std::string filename, bool load_from_filesystem) :
	done(false)
{
	if (internal::audio_context.mute) {
		spec = NULL;
		file = NULL;
		data = NULL;
		return;
	}

	do_free = false;

#if defined USE_VORBIS
	if (filename.find(".ogg") != std::string::npos) {
		if (load_from_filesystem) {
			file = SDL_IOFromFile(filename.c_str(), "r");
		}
		else {
			filename = "audio/samples/" + filename;
			file = util::open_file(filename, 0);
		}

		char errmsg[1000];

		spec = new SDL_AudioSpec;
		Uint32 size;

		data = audio::decode_vorbis(file, errmsg, spec, &size);
		
		if (load_from_filesystem) {
			SDL_CloseIO(file);
		}
		else {
			util::close_file(file);
		}

		file = nullptr;

		if (data == 0) {
			delete spec;
			throw util::Error(errmsg);
		}

		length = size / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);

		file = 0;
	}
	else
#endif
#if defined USE_FLAC
	if (filename.find(".flac") != std::string::npos) {
		if (load_from_filesystem) {
			file = SDL_IOFromFile(filename.c_str(), "r");
			if (file == nullptr) {
				throw util::LoadError("Error loading " + filename);
			}
		}
		else {
			filename = "audio/samples/" + filename;
			file = util::open_file(filename, 0);
		}

		char errmsg[1000];

		spec = new SDL_AudioSpec;
		Uint32 size;

		data = audio::decode_flac(file, errmsg, spec, &size);
		
		if (load_from_filesystem) {
			SDL_CloseIO(file);
		}
		else {
			util::close_file(file);
		}

		file = nullptr;

		if (data == 0) {
			delete spec;
			util::debugmsg(errmsg);
			throw util::Error(errmsg);
		}

		length = size / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);

		file = 0;
	}
	else
#endif
	{
		if (load_from_filesystem) {
			file = SDL_IOFromFile(filename.c_str(), "r");
		}
		else {
			filename = "audio/samples/" + filename;
			file = util::open_file(filename, 0);
		}

		spec = new SDL_AudioSpec;

		Uint8 *buf;

		if (SDL_LoadWAV_IO(file, false, spec, &buf, &length) == 0) {
			util::close_file(file);
			throw util::LoadError("SDL_LoadWAV_IO failed");
		}

		int orig_len = length;
		length = length / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);

		SDL_AudioFormat out_format;
		bool _16bit_samples = true;
		util::JSON::Node *root = shim::shim_json->get_root();
		_16bit_samples = root->get_nested_bool("shim>audio>16bit_samples", nullptr, false, true, true);
		if (util::bool_arg(_16bit_samples, shim::argc, shim::argv, "16bit-samples")) {
			out_format = SDL_AUDIO_S16LE;
		}
		else {
			out_format = SDL_AUDIO_F32LE;
		}

		int out_len = length*spec->channels*(SDL_AUDIO_BITSIZE(out_format)/8);
		data = new Uint8[out_len];

		SDL_AudioSpec out_spec;
		out_spec.format = out_format;
		out_spec.freq = internal::audio_context.device_spec.freq;
		out_spec.channels = 2;

		SDL_ConvertAudioSamples(spec, buf, orig_len, &out_spec, &data, &out_len);

		if (load_from_filesystem) {
			SDL_CloseIO(file);
		}
		else {
			util::close_file(file);
		}
		file = nullptr;

		spec->format = out_format;
		spec->channels = 2;
		spec->freq = internal::audio_context.device_spec.freq;

		do_free = true;
	}
}

void Sample::delete_instances()
{
	while (true) {
		bool done = true;
		std::vector<Sample_Instance *>::iterator it;
		for (it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
			Sample_Instance *s = *it;
			if (s->spec == spec) {
				done = false;
				Sample::stop_instance(s);
				break;
			}
		}
		if (done) {
			break;
		}
	}
}

Sample::~Sample()
{
	delete_instances();
	if (do_free) {
		SDL_free(data);
	}
	else {
		delete[] data;
	}
	delete spec;
}

void Sample::play(bool loop)
{
	play(1.0f, loop);
}

// FIXME: avoid repetition here with other play method
void Sample::play(float volume, bool loop, int type)
{
	if (internal::audio_context.mute) {
		return;
	}

	Sample_Instance *s = new Sample_Instance();
	if (s == 0) {
		return;
	}

	done = false;

	float p = (float)internal::audio_context.device_spec.freq / spec->freq;

	s->spec = spec;
	s->data = data;
	s->length = length;
	s->play_length = length * p;
	s->offset = 0;
	s->silence = 0;
	s->loop = loop;
	s->volume = volume;
	s->sample = this;
	s->bits_per_sample = SDL_AUDIO_BITSIZE(spec->format);
	s->bytes_per_sample = s->bits_per_sample / 8;
	s->format_is_float = SDL_AUDIO_ISFLOAT(spec->format);
	s->format_is_signed = SDL_AUDIO_ISSIGNED(spec->format);
	s->format_should_be_swapped = (SDL_AUDIO_ISBIGENDIAN(spec->format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) || (SDL_AUDIO_ISLITTLEENDIAN(spec->format) && SDL_BYTEORDER == SDL_BIG_ENDIAN);
	if (s->format_is_float) {
		s->min_sample = -1.0f;
		s->max_sample = 1.0f;
	}
	else {
		s->min_sample = -powf(2, s->bits_per_sample-1);
		s->max_sample = powf(2, s->bits_per_sample-1) - 1;
	}

	s->type = type;
	s->master_volume = 1.0f;
	s->channels = spec->channels;

	audio::lock_mutex();
	delete_instances();
	internal::audio_context.playing_samples.push_back(s);
	audio::unlock_mutex();
}

void Sample::play(float volume, bool loop)
{
	play(volume, loop, 0);
}

Sample_Instance *Sample::play_stretched(float volume, Uint32 silence, Uint32 play_length, int type)
{
	if (internal::audio_context.mute) {
		return 0;
	}

	Sample_Instance *s = new Sample_Instance();
	if (s == 0) {
		return 0;
	}

	done = false;

	int bits_per_sample = SDL_AUDIO_BITSIZE(spec->format);
	int bytes_per_sample = bits_per_sample / 8;
	float p = (float)internal::audio_context.device_spec.freq / spec->freq;

	s->spec = spec;
	s->data = data;
	s->length = length;
	s->play_length = (play_length == 0 ? length * p : play_length);
	s->offset = 0;
	s->silence = silence;
	s->loop = false;
	s->volume = volume;
	s->sample = this;
	s->bits_per_sample = bits_per_sample;
	s->bytes_per_sample = bytes_per_sample;
	s->format_is_float = SDL_AUDIO_ISFLOAT(spec->format);
	s->format_is_signed = SDL_AUDIO_ISSIGNED(spec->format);
	s->format_should_be_swapped = (SDL_AUDIO_ISBIGENDIAN(spec->format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) || (SDL_AUDIO_ISLITTLEENDIAN(spec->format) && SDL_BYTEORDER == SDL_BIG_ENDIAN);
	if (s->format_is_float) {
		s->min_sample = -1.0f;
		s->max_sample = 1.0f;
	}
	else {
		s->min_sample = -powf(2, s->bits_per_sample-1);
		s->max_sample = powf(2, s->bits_per_sample-1) - 1;
	}

	s->type = type;
	s->master_volume = 1.0f;
	s->channels = spec->channels;
	
	audio::lock_mutex();
	internal::audio_context.playing_samples.push_back(s);
	audio::unlock_mutex();

	return s;
}

bool Sample::is_done()
{
	return done;
}

void Sample::set_done(bool done)
{
	this->done = done;
}

// We don't return sample instances to the user (we could) so stop has to stop all instances of this sample
void Sample::stop_all()
{
	audio::lock_mutex();

	std::vector<Sample_Instance *>::iterator it;

	for (it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end();) {
		Sample_Instance *s = *it;
		if (s->data == data) {
			it = internal::audio_context.playing_samples.erase(it);
			delete s;
		}
		else {
			it++;
		}
	}

	done = false;

	audio::unlock_mutex();
}

void Sample::stop()
{
	stop_all();
}

bool Sample::is_playing()
{
	bool playing = false;

	audio::lock_mutex();

	std::vector<Sample_Instance *>::iterator it;

	for (it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
		Sample_Instance *s = *it;
		if (s->data == data) {
			playing = true;
			break;
		}
	}

	audio::unlock_mutex();

	return playing;
}

Uint32 Sample::get_length()
{
	return length;
}

int Sample::get_frequency()
{
	return spec->freq;
}

} // End namespace audio

} // End namespace noo
