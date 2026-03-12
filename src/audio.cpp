#include "shim5/audio.h"
#include "shim5/json.h"
#include "shim5/mml.h"
#include "shim5/sample.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/util.h"

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
static bool sfx_paused;
static math::Interpolator *hermite;

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
	SDL_LockAudioStream(audio_stream);

	Uint8 *stream = SDL_stack_alloc(Uint8, additional_amount);

	int samples = additional_amount / format_bytes / audio::internal::audio_context.device_spec.channels; // 2 channels -- will be repeated to make stereo

	for (int i = 0; i < samples*audio::internal::audio_context.device_spec.channels; i++) {
		music_buf[i] = 0.0f;
		sfx_buf[i] = 0.0f;
	}

	int max = audio::MML::mix(music_buf, samples, sfx_paused);

	std::vector<audio::Sample_Instance *>::iterator it;
	for (it = audio::internal::audio_context.playing_samples.begin(); it != audio::internal::audio_context.playing_samples.end();) {
		audio::Sample_Instance *s = *it;
		int count = s->silence;
		s->silence -= MIN((int)s->silence, samples);
		while (count < samples) {
			int length;
			float p;

			bool interpolate;

			if (s->play_length != s->length) {
				length = s->play_length - s->offset;
				if (length > samples-count) {
					length = samples - count;
				}

				p = (float)s->play_length / s->length;

				interpolate = true;
			}
			else {
				length = s->length - s->offset;
				if (length > samples-count) {
					length = samples - count;
				}

				p = 1.0f;

				interpolate = false;
			}

			max = MAX(max, length);

			for (int i = 0; i < length; i++) {
				float sample_offset_f = (i + s->offset) / p * s->spec->channels;
				int sample_offset = int((i + s->offset) / p) * s->spec->channels;
				if (sample_offset <= 1 || sample_offset >= (int)s->length) {
					// special case because we can't access the previous sample below (segfault)
					interpolate = false;
				}

				int loops;
				if (s->spec->channels == 2 && audio::internal::audio_context.device_spec.channels == 2) {
					loops = 2;
				}
				else {
					loops = 1;
				}

				for (int k = 0; k < loops; k++) {
					float v;
					if (interpolate) {
						int samps[4];
						float values[4];
						samps[1] = sample_offset;
						samps[0] = samps[1] - s->spec->channels;
						samps[2] = samps[1] + s->spec->channels;
						samps[3] = samps[1] + s->spec->channels*2;
						for (int i = 0; i < 4; i++) {
							if (samps[i] < 0) {
								samps[i] = 0;
							}
							else if ((Uint32)samps[i] >= s->length*s->spec->channels) {
								samps[i] = s->length*s->spec->channels - 1;
							}
							values[i] = read_float_sample(s, samps[i]);
						}
						hermite->start(values[0], values[1], values[2], values[3], 1000000);
						float f = fmodf(sample_offset_f, s->spec->channels);
						f /= s->spec->channels;
						hermite->interpolate(f * 1000000);
						v = hermite->get_value();
					}
					else {
						v = read_float_sample(s, sample_offset);
					}
					v = v * s->volume * s->master_volume;

					int dest_offset = (count + i) * audio::internal::audio_context.device_spec.channels + k;

					if (audio::internal::audio_context.device_spec.channels == 2 && s->spec->channels == 1) {
						*((float *)sfx_buf + dest_offset) += v;
						*((float *)sfx_buf + dest_offset+1) += v;
					}
					else {
						*((float *)sfx_buf + dest_offset) += v;
					}

					sample_offset++;
				}
			}

			s->offset += length;

			if (s->loop && s->offset >= s->play_length) {
				s->offset = 0;
			}

			if (s->loop) {
				count += length;
			}
			else {
				break;
			}
		}
		if (s->loop == false && s->offset >= s->play_length) {
			s->sample->set_done(true);
			// erasing causes a memory leak
			it++;// = audio::internal::audio_context.playing_samples.erase(it);
		}
		else {
			it++;
		}
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

#ifdef DUMP
	for (int i = 0; i < max*audio::internal::audio_context.device_spec.channels; i++) {
		float v = (music_buf[i] + sfx_buf[i]);
		if (v < -1.0f) {
			v = -1.0f;
		}
		else if (v > 1.0f) {
			v = 1.0f;
		}
		SDL_WriteU16LE(dumpfile, v*32767);
	}
#endif

	SDL_PutAudioStreamData(audio_stream, stream, additional_amount);

	SDL_stack_free(stream);

	SDL_UnlockAudioStream(audio_stream);
}

namespace noo {

namespace audio {

bool static_start()
{
	internal::audio_context.playing_samples.clear();
	sfx_paused = false;

	return true;
}

bool start()
{
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
		internal::audio_context.device_spec.channels = 2;

		audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &internal::audio_context.device_spec, audio_callback, nullptr);

		if (audio_stream == 0) {
			internal::audio_context.mute = false;
			util::infomsg("audio::start failed: %s\n", SDL_GetError());
			return false;
		}

		shim::samplerate = internal::audio_context.device_spec.freq;

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

		util::infomsg("Audio format=0x%x, frequency=%d Hz\n", format, internal::audio_context.device_spec.freq);

		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio_stream));
	}

	music_buf = new float[SHIM_AUDIO_BUFFER_SIZE*internal::audio_context.device_spec.channels];
	sfx_buf = new float[SHIM_AUDIO_BUFFER_SIZE*internal::audio_context.device_spec.channels];

	hermite = new math::I_Hermite();
	
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

	delete hermite;
	hermite = nullptr;

	MML::static_stop();
}

int millis_to_samples(int millis)
{
	float f = millis / 1000.0f;
	return internal::audio_context.device_spec.freq * f;
}

int samples_to_millis(int samples, int freq)
{
	return samples / (freq == -1 ? (float)internal::audio_context.device_spec.freq : freq) * 1000.0f;
}

void pause_sfx(bool paused)
{
	sfx_paused = paused;
}

void lock_mutex()
{
	SDL_LockAudioStream(audio_stream);
}

void unlock_mutex()
{
	SDL_UnlockAudioStream(audio_stream);
}

namespace internal {

Audio_Context audio_context;

} // End namespace internal

} // End namespace audio

} // End namespace noo
