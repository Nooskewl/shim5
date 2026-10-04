#include "shim5/audio.h"
#include "shim5/json.h"
#include "shim5/mml.h"
#include "shim5/sample.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/util.h"
#include "shim5/flac.h"
#include "shim5/vorbis.h"

#include "shim5/internal/audio.h"

//#define DUMP

#ifdef DUMP
SDL_IOStream *dumpfile;
#endif

using namespace noo;

static float *music_buf;
static float *sfx_buf;
static SDL_AudioStream *audio_stream;
static SDL_AudioFormat format;
static int format_bits;
static int format_bytes;
static bool format_is_float;
static bool format_is_signed;
static bool format_should_be_swapped;
static float min_sample;
static float max_sample;
static SDL_Mutex *mutex;

static float swap_float(float f)
{
	union {
		uint32_t u;
		float f;
	} u;
	u.f = f;
	u.u = SDL_Swap32(u.u);
	return u.f;
}

static int16_t swap_signed16(int16_t i)
{
	union {
		uint16_t u;
		int16_t i;
	} u;
	u.i = i;
	u.u = SDL_Swap16(u.u);
	return u.i;
}

static int32_t swap_signed32(int32_t i)
{
	union {
		uint32_t u;
		int32_t i;
	} u;
	u.i = i;
	u.u = SDL_Swap32(u.u);
	return u.i;
}

static void write_sample(Uint8 *stream, int i/*sample*/, float v/*min_sample->max_sample*/)
{
	if (format_is_float) {
		if (format_bits == 32) {
			if (format_should_be_swapped) {
				v = swap_float(v);
			}
			*((float *)stream + i) = v;
		}
		else {
			throw util::Error("Only 32 bit floating point samples are supported!");
		}
	}
	else {
		if (format_bits == 8) {
			if (format_is_signed) {
				*((signed char *)stream + i) = v;
			}
			else {
				*((unsigned char *)stream + i) = v;
			}
		}
		else if (format_bits == 16) {
			if (format_is_signed) {
				int16_t ii = v;
				if (format_should_be_swapped) {
					ii = swap_signed16(ii);
				}
				*((int16_t *)stream + i) = ii;
			}
			else {
				uint16_t u = v;
				if (format_should_be_swapped) {
					u = SDL_Swap16(u);
				}
				*((uint16_t *)stream + i) = u;
			}
		}
		else if (format_bits == 32) {
			if (format_is_signed) {
				int32_t ii = v;
				if (format_should_be_swapped) {
					ii = swap_signed32(ii);
				}
				*((int32_t *)stream + i) = ii;
			}
			else {
				throw util::Error("Unsigned 32 bit samples not supported!");
			}
		}
		else {
			throw util::Error("Only 8, 16 and 32 bit samples are supported!");
		}
	}
}

static float read_float_sample(audio::Sample_Instance *s, int sample)
{
	if (s->format_is_float) {
		float v = *((float *)s->data + sample);
		if (s->format_should_be_swapped) {
			v = swap_float(v);
		}
		return v;
	}
	else {
		float v;
		if (s->bits_per_sample == 8) {
			if (s->format_is_signed) {
				v = *((signed char *)s->data + sample);
			}
			else {
				v = *((unsigned char *)s->data + sample);
			}
		}
		else if (s->bits_per_sample == 16) {
			if (s->format_is_signed) {
				v = *((int16_t *)s->data + sample);
			}
			else {
				v = *((uint16_t *)s->data + sample);
			}
		}
		else if (s->bits_per_sample == 32) {
			if (s->format_is_signed) {
				v = *((int32_t *)s->data + sample);
			}
			else {
				throw util::Error("Unsigned 32 bit samples not supported!");
			}
		}
		else {
			throw util::Error(util::string_printf("Sample has %d bits (unsupported!)", s->bits_per_sample));
		}
		if (s->format_is_signed == false) {
			v -= s->max_sample;
		}
		return v / s->max_sample;
	}
}

// Mixes samples and MML into the audio device buffer
static void audio_callback(void *userdata, SDL_AudioStream *audio_stream, int additional_amount, int total_amount)
{
	audio::lock_mutex();

	Uint8 *stream = SDL_stack_alloc(Uint8, additional_amount);

	int samples = additional_amount / format_bytes / audio::internal::audio_context.device_spec.channels;

	for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
		music_buf[i] = 0.0f;
		sfx_buf[i] = 0.0f;
	}

	for (int samp = 0; samp < samples; samp++) {
		std::vector<audio::Sample_Instance *>::iterator it;
		for (it = audio::internal::audio_context.playing_samples.begin(); it != audio::internal::audio_context.playing_samples.end();) {
			audio::Sample_Instance *s = *it;
			if (s->paused) {
				it++;
				continue;
			}
			else if (s->loop == false && s->offset >= s->play_length) {
				it++;
				continue;
			}
			else if (s->silence > 0) {
				s->silence--;
			}
			else {
				float p;

				if (s->play_length != s->length) {
					p = (float)s->play_length / s->length;
				}
				else {
					p = 1.0f;
				}

				int sample_offset = int(s->offset / p) * s->spec->channels;

				float v;
				int dest_offset;

				if (audio::internal::audio_context.device_spec.channels == 2) {
					if (s->spec->channels >= 2) {
						v = read_float_sample(s, sample_offset);
						v = v * s->volume * audio::calc_pan_left(s->pan);
						dest_offset = samp * audio::internal::audio_context.device_spec.channels + 0;
						*((float *)sfx_buf + dest_offset) += v;
						sample_offset++;
						v = read_float_sample(s, sample_offset);
						v = v * s->volume * audio::calc_pan_right(s->pan);
						dest_offset = samp * audio::internal::audio_context.device_spec.channels + 1;
						*((float *)sfx_buf + dest_offset) += v;
						sample_offset++;
					}
					else {
						v = read_float_sample(s, sample_offset);
						v = v * s->volume;
						dest_offset = samp * audio::internal::audio_context.device_spec.channels + 0;
						*((float *)sfx_buf + dest_offset) += v;
						sample_offset++;
						dest_offset = samp * audio::internal::audio_context.device_spec.channels + 1;
						*((float *)sfx_buf + dest_offset) += v;
						sample_offset++;
					}
				}
				else {
					if (s->spec->channels >= 2) {
						v = read_float_sample(s, sample_offset);
						v = v * s->volume * audio::calc_pan_left(s->pan);
						sample_offset++;
						float v2 = read_float_sample(s, sample_offset);
						v2 = v2 * s->volume * audio::calc_pan_right(s->pan);
						sample_offset++;
						dest_offset = samp * audio::internal::audio_context.device_spec.channels + 0;
						*((float *)sfx_buf + dest_offset) += (v + v2);
					}
					else {
						v = read_float_sample(s, sample_offset);
						v = v * s->volume;
						dest_offset = samp * audio::internal::audio_context.device_spec.channels + 0;
						*((float *)sfx_buf + dest_offset) += v;
						sample_offset++;
					}
				}

				s->offset++;

				if (s->loop && s->offset >= s->play_length) {
					s->offset = 0;
				}
			}
			if (s->offset >= s->play_length) {
				if (s->finished_callback && std::find(audio::internal::audio_callback_data.begin(), audio::internal::audio_callback_data.end(), s->finished_callback_data) == audio::internal::audio_callback_data.end()) {
					audio::internal::audio_callbacks.push_back(s->finished_callback);
					audio::internal::audio_callback_data.push_back(s->finished_callback_data);
					s->finished_callback = nullptr;
				}
			}
			if (s->loop == false && s->offset >= s->play_length) {
				//it++; // erase causes leak
				it = audio::internal::audio_context.playing_samples.erase(it);
				if (s->mml) {
					s->mml->delete_wavs(s);
				}
				delete s;
											      
			}
			else {
				it++;
			}
		}
		audio::MML::mix(music_buf+samp*audio::internal::audio_context.device_spec.channels, 1);
	}

	// Fast paths for common sample formats...
	if (format == SDL_AUDIO_F32LE && format_should_be_swapped == false) {
		for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
			float v = (music_buf[i] + sfx_buf[i]);
			if (v < -1.0f) {
				v = -1.0f;
			}
			else if (v > 1.0f) {
				v = 1.0f;
			}
			*((float *)stream + i) = v;
		}
	}
	else if (format == SDL_AUDIO_S16LE && format_should_be_swapped == false) {
		for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
			float v = (music_buf[i] + sfx_buf[i]) * max_sample;
			if (v < min_sample) {
				v = min_sample;
			}
			else if (v > max_sample) {
				v = max_sample;
			}
			*((int16_t *)stream + i) = v;
		}
	}
	// generic conversion...
	else {
		for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
			float v = (music_buf[i] + sfx_buf[i]) * max_sample;
			if (format_is_signed == false) {
				v += -min_sample;
				if (v < 0.0f) {
					v = 0.0f;
				}
				else if (v > max_sample-min_sample) { // subtracting negative = adding positive
					v = max_sample-min_sample;
				}
			}
			else {
				if (v < min_sample) {
					v = min_sample;
				}
				else if (v > max_sample) {
					v = max_sample;
				}
			}
			write_sample(stream, i, v);
		}
	}

	SDL_PutAudioStreamData(audio_stream, stream, additional_amount);

	SDL_stack_free(stream);

	audio::unlock_mutex();
}

namespace noo {

namespace audio {

bool static_start()
{
	internal::audio_context.playing_samples.clear();

	Sample::register_sample_loader("wav", decode_wav);
	Sample::register_sample_loader("flac", decode_flac);
	Sample::register_sample_loader("ogg", decode_vorbis);

	return true;
}

bool start()
{
	mutex = SDL_CreateMutex();

	util::JSON::Node *root = shim::shim_json->get_root();

	internal::audio_context.mute = root->get_nested_bool("shim>audio>mute", &internal::audio_context.mute, false, true, true);
	internal::audio_context.mute = util::bool_arg(internal::audio_context.mute, shim::argc, shim::argv, "mute");

	int arg;

	internal::audio_context.device_spec.freq = root->get_nested_int("shim>audio>freq", &internal::audio_context.device_spec.freq, 48000, true, true);
	if ((arg = util::check_args(shim::argc, shim::argv, "+freq")) > 0) {
		internal::audio_context.device_spec.freq = atoi(shim::argv[arg+1]);
	}
	bool float_samples = false;
	float_samples = root->get_nested_bool("shim>audio>float_samples", nullptr, false, true, true);
	if (util::bool_arg(float_samples, shim::argc, shim::argv, "float-samples")) {
		internal::audio_context.device_spec.format = SDL_AUDIO_F32LE;
	}
	else {
		internal::audio_context.device_spec.format = SDL_AUDIO_S16LE;
	}

	if (internal::audio_context.mute == false) {
		bool mono_audio = false;
		mono_audio = root->get_nested_bool("shim>audio>mono_audio", nullptr, false, true, true);
		if (util::bool_arg(mono_audio, shim::argc, shim::argv, "mono-audio")) {
			internal::audio_context.device_spec.channels = 1;
		}
		else {
			internal::audio_context.device_spec.channels = 2;
		}

		audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &internal::audio_context.device_spec, audio_callback, nullptr);

		if (audio_stream == 0) {
			internal::audio_context.mute = false;
			util::infomsg("audio::start failed: %s\n", SDL_GetError());
			return false;
		}

		// this stuff allows converting between formats
		format =  internal::audio_context.device_spec.format;
		format_bits = SDL_AUDIO_BITSIZE(format);
		format_bytes = format_bits / 8;
		format_is_float = SDL_AUDIO_ISFLOAT(format);
		format_is_signed = SDL_AUDIO_ISSIGNED(format);
		format_should_be_swapped = (SDL_AUDIO_ISBIGENDIAN(format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) || (SDL_AUDIO_ISLITTLEENDIAN(format) && SDL_BYTEORDER == SDL_BIG_ENDIAN);
		if (format_is_float) {
			min_sample = -1.0f;
			max_sample = 1.0f;
		}
		else {
			min_sample = -powf(2, format_bits-1);
			max_sample = powf(2, format_bits-1) - 1;
		}

		//util::infomsg("Audio format=0x%x, frequency=%d Hz\n", format, internal::audio_context.device_spec.freq);
	}

	music_buf = new float[SHIM_AUDIO_BUFFER_SIZE*internal::audio_context.device_spec.channels];
	sfx_buf = new float[SHIM_AUDIO_BUFFER_SIZE*internal::audio_context.device_spec.channels];

	if (internal::audio_context.mute == false) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio_stream));
	}

	MML::static_start(); // this can't go in audio::static_start because it needs some device info

	return true;
}

void stop_all_samples()
{
	std::vector<Sample_Instance *> &v = internal::audio_context.playing_samples;
	while (v.size() > 0) {
		Sample_Instance *s = v[0];
		Sample::stop_instance(s); // this locks mutex
	}
}

void end()
{
	stop_all_samples();
	if (audio_stream != 0) {
		SDL_DestroyAudioStream(audio_stream);
	}

	delete[] music_buf;
	delete[] sfx_buf;
	music_buf = nullptr;
	sfx_buf = nullptr;

	MML::static_end();
	Sample::static_end();

	SDL_DestroyMutex(mutex);
}

int millis_to_samples(int millis, int freq)
{
	float f = millis / 1000.0f;
	return (freq == -1 ? internal::audio_context.device_spec.freq : freq) * f;
}

int samples_to_millis(int samples, int freq)
{
	return (float)samples / (freq == -1 ? internal::audio_context.device_spec.freq : freq) * 1000.0f;
}

float calc_pan_left(float pan)
{
	if (pan <= 0) {
		return 1.0f;
	}
	return 1.0f - pan;
}

float calc_pan_right(float pan)
{
	if (pan >= 0) {
		return 1.0f;
	}
	return 1.0f + pan;
}

void lock_mutex()
{
	SDL_LockMutex(mutex);
}

void unlock_mutex()
{
	SDL_UnlockMutex(mutex);
}

namespace internal {

Audio_Context audio_context;
std::vector<util::Callback> audio_callbacks;
std::vector<void *> audio_callback_data;

} // End namespace internal

} // End namespace audio

} // End namespace noo
