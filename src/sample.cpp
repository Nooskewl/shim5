#include "shim5/audio.h"
#include "shim5/flac.h"
#include "shim5/json.h"
#include "shim5/mml.h"
#include "shim5/sample.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/vorbis.h"
#include "shim5/util.h"

#include "shim5/internal/audio.h"

using namespace noo;

namespace noo {

namespace audio {

static std::map<std::string, sample_loader> sample_loaders;

void Sample::register_sample_loader(std::string ext, sample_loader func)
{
	sample_loaders[ext] = func;
}

void Sample::stop_instance(Sample_Instance *s)
{
	audio::lock_mutex();

	for (std::vector<Sample_Instance *>::iterator it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
		Sample_Instance *s2 = *it;
		if (s2 == s) {
			internal::audio_context.playing_samples.erase(it);
			if (s->mml) {
				s->mml->delete_wavs(s);
			}
			delete s;
			break;
		}
	}

	audio::unlock_mutex();
}

void Sample::set_instance_volume(Sample_Instance *s, float volume)
{
	audio::lock_mutex();

	s->volume = volume;

	audio::unlock_mutex();
}

void Sample::pause_instance(Sample_Instance *s, bool onoff)
{
	audio::lock_mutex();

	s->paused = onoff;

	audio::unlock_mutex();
}

bool Sample::sample_active(Sample_Instance *s)
{
	audio::lock_mutex();

	for (std::vector<Sample_Instance *>::iterator it = internal::audio_context.playing_samples.begin(); it != internal::audio_context.playing_samples.end(); it++) {
		Sample_Instance *s2 = *it;
		if (s2 == s) {
			audio::unlock_mutex();
			return true;
		}
	}

	audio::unlock_mutex();

	return false;
}

void Sample::set_finished_callback(Sample_Instance *s, util::Callback callback, void *callback_data)
{
	s->finished_callback = callback;
	s->finished_callback_data = callback_data;
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

	std::pair<std::string, sample_loader> p;
	std::map<std::string, sample_loader>::iterator it;
	size_t loc = filename.rfind('.');
	std::string ext;
	if (loc != std::string::npos) {
		ext = filename.substr(loc+1);
		ext = util::lowercase(ext);
		it = sample_loaders.find(ext);
		if (it == sample_loaders.end()) {
			ext = "wav";
		}
	}
	else {
		ext = "wav";
	}
	data = sample_loaders[ext](file, errmsg, spec, &size);
	
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
}

Sample::Sample(Uint8 *data, int size, int freq, int channels) :
	done(false)
{
	if (internal::audio_context.mute) {
		spec = NULL;
		file = NULL;
		data = NULL;
		return;
	}

	spec = new SDL_AudioSpec;

	spec->format = SDL_AUDIO_S16LE;
	spec->channels = channels;
	spec->freq = freq;

	file = nullptr;
	this->data = data;

	length = size / spec->channels / (SDL_AUDIO_BITSIZE(spec->format)/8);
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
	audio::lock_mutex();
	delete_instances();
	audio::unlock_mutex();
	delete[] data;
	delete spec;
}

// FIXME: avoid repetition here with other play method
void Sample::play(float volume, bool loop, float pan)
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

	s->finished_callback = nullptr;
	s->finished_callback_data = nullptr;

	s->spec = spec;
	s->data = data;
	s->length = length;
	s->play_length = length * p;
	s->offset = 0;
	s->silence = 0;
	s->loop = loop;
	s->volume = volume;
	s->pan = pan;
	s->sample = this;
	s->mml = nullptr;
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
	s->paused = false;

	audio::lock_mutex();
	delete_instances();
	internal::audio_context.playing_samples.push_back(s);
	audio::unlock_mutex();
}

Sample_Instance *Sample::play_stretched(float volume, Uint32 silence, Uint32 play_length, bool loop, float pan, util::Callback finished_callback, void *finished_callback_data)
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

	s->finished_callback = finished_callback;
	s->finished_callback_data = finished_callback_data;

	s->spec = spec;
	s->data = data;
	s->length = length;
	s->play_length = (play_length == 0 ? length * p : play_length);
	s->offset = 0;
	s->silence = silence;
	s->loop = loop;
	s->volume = volume;
	s->pan = pan;
	s->sample = this;
	s->mml = nullptr;
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
	s->paused = false;

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
			if (s->mml) {
				s->mml->delete_wavs(s);
			}
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

Uint8 *Sample::get_data()
{
	return data;
}

SDL_AudioSpec *Sample::get_spec()
{
	return spec;
}

Uint8 *decode_wav(SDL_IOStream *file, char *errmsg, SDL_AudioSpec *spec, Uint32 *size)
{
	Uint8 *buf;

	if (SDL_LoadWAV_IO(file, false, spec, &buf, size) == 0) {
		util::close_file(file);
		throw util::LoadError("SDL_LoadWAV_IO failed");
	}

	SDL_AudioFormat out_format = SDL_AUDIO_S16LE;

	SDL_AudioSpec out_spec;
	out_spec.format = out_format;
	out_spec.freq = internal::audio_context.device_spec.freq;
	out_spec.channels = 2;

	int out_len;
	Uint8 *d;

	SDL_ConvertAudioSamples(spec, buf, *size, &out_spec, &d, &out_len);

	spec->format = out_format;
	spec->channels = 2;
	spec->freq = internal::audio_context.device_spec.freq;

	*size = out_len;

	Uint8 *data = new Uint8[out_len];

	memcpy(data, d, out_len);

	SDL_free(buf);
	SDL_free(d);

	return data;
}

} // End namespace audio

} // End namespace noo
