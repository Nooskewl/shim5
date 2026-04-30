#ifndef NOO_MML_H
#define NOO_MML_H

#include "shim5/main.h"
#include "shim5/interp.h"

namespace noo {

namespace audio {

class Sample;
struct Sample_Instance;

class MML {
public:
	enum Wave_Type {
		PULSE = 0,
		NOISE,
		SAWTOOTH,
		SINE,
		TRIANGLE,
		NOISE_ORIG,
	};

	struct Wav_Start {
		int sample;
		Uint32 orig_play_start;
		Uint32 play_start;
		Sample_Instance *instance;
		float volume;
		std::vector<std::string> toks;
		int note_length;
		int orig_tempo;
		int tempo;
		int octave;
		int note;
		int length;
	};

	struct Reverb_Type {
		int reverberations;
		int falloff_interpolator;
		int falloff_time;
		int start_volume;
		int final_volume;
	};

	struct Track_Data {
		std::string text;
		Wave_Type type;
		std::vector< std::pair<int, float> > volumes;
		std::vector< std::pair<int, float> > volume_offsets;
		std::vector<int> pitches;
		std::vector<int> pitch_offsets;
		std::vector< std::pair<int, float> > dutycycles;
		std::vector< std::pair<int, float> > pans;
		int pad;
		std::vector<Wav_Start> wav_starts;
		Uint32 beginning_silence;
	};

	struct MML_Data {
		MML *mml;
		std::vector< std::vector<float> > pitch_envelopes;
		std::vector< std::vector<float> > pitch_offset_envelopes;
		std::vector<Reverb_Type> reverb_types;
		std::vector<Track_Data *> track_data;
	};

	static void static_start();
	static void static_stop();
	static int mix(float *buf, int samples);

	SHIM5_EXPORT MML(SDL_IOStream *f, bool load_from_filesystem = false);
	SHIM5_EXPORT MML(std::string filename, bool load_from_filesystem = false);
	SHIM5_EXPORT virtual ~MML();

	SHIM5_EXPORT Uint32 play(float volume = 1.0f, bool loop = false, float pan = 0.0f, util::Callback finished_callback = 0, void *finished_callback_data = 0);
	SHIM5_EXPORT void stop(Uint32 id);
	SHIM5_EXPORT void pause(Uint32 id, bool onoff);
	SHIM5_EXPORT void set_master_volume(Uint32 id, float volume);
	SHIM5_EXPORT float get_master_volume(Uint32 id);
	SHIM5_EXPORT void set_tempo(Uint32 id, int bpm);
	SHIM5_EXPORT std::string get_name(); // returns same thing passed to constructor
	SHIM5_EXPORT bool track_active(Uint32 id);
	SHIM5_EXPORT float get_pan(Uint32 id);
	SHIM5_EXPORT void set_pan(Uint32 id, float pan);

private:
	class Track
	{
	public:
		// pad is # of samples of silence to pad the end with so all tracks are even
		SHIM5_EXPORT Track(Uint32 id, Wave_Type type, std::string text, std::vector< std::pair<int, float> > &volumes, std::vector< std::pair<int, float> > &volume_offsets, std::vector<int> &pitches, std::vector<int> &pitch_offsets, std::vector< std::vector<float> > &pitch_envelopes, std::vector< std::vector<float> > &pitch_offset_envelopes, std::vector< std::pair<int, float> > &dutycycles, int pad, std::vector<Sample *> wav_samples, std::vector<Wav_Start> wav_starts, Uint32 beginning_silence, MML *mml, std::vector<Reverb_Type> reverb_types, std::vector< std::pair<int, float> > &pans);
		~Track();

		SHIM5_EXPORT void play(bool loop);
		SHIM5_EXPORT void stop();
		SHIM5_EXPORT void pause(bool onoff);
		SHIM5_EXPORT int update(float *buf, int length);

		SHIM5_EXPORT bool is_playing();
		SHIM5_EXPORT bool is_done();

		SHIM5_EXPORT void set_master_volume(float master_volume);
		SHIM5_EXPORT float get_master_volume();
		SHIM5_EXPORT float get_master_volume_real();

		SHIM5_EXPORT Uint32 get_id();

		SHIM5_EXPORT void real_set_tempo(int bpm);
		SHIM5_EXPORT void set_tempo(int bpm);
		SHIM5_EXPORT int get_new_tempo();

		SHIM5_EXPORT void set_callbacks(util::Callback finished_callback, void *finished_callback_data);
		SHIM5_EXPORT void call_callbacks();

		SHIM5_EXPORT float get_pan(); // get global pan
		SHIM5_EXPORT void set_pan(float pan); // set global pan

	private:
		void reset(Uint32 buffer_fulfilled);

		float vol_from_phase(float p, MML::Wave_Type type, float freq, float dutycycle);
		void generate(float *buf, int samples, int t, const char *tok, int octave);

		void real_get_frequency(int index, std::vector< std::vector<float> > &v, float zero_freq, float default_frequency, float &ret_freq, float &ret_time, float &ret_len, float &last_freq, float &last_start, int &same_sections, bool offset);
		void get_frequency(float start_freq, float &ret_freq, float &ret_time, float &ret_len);
		void get_frequency_offset(float &ret_freq, float &ret_time, float &ret_len);
		float real_get_volume(int &section, std::vector< std::pair<int, float> > &v, bool offset);
		float get_volume();
		float get_dutycycle();
		float calc_pan();
		void start_wavs(Uint32 buffer_offset, Uint32 on_or_after);
		void stop_wavs();

		std::string next_note(const char *text, int *pos);
		int notelength(const char *tok, const char *text, int *pos);

		Uint32 id;

		Wave_Type type;
		std::string text;
		std::vector< std::pair<int, float> > volumes;
		std::vector< std::pair<int, float> > volume_offsets;
		std::vector<int> pitches;
		std::vector<int> pitch_offsets;
		std::vector< std::vector<float> > pitch_envelopes;
		std::vector< std::vector<float> > pitch_offset_envelopes;
		std::vector< std::pair<int, float> > dutycycles;
		std::vector< std::pair<int, float> > pans;

		int pad;
		int sample;
		int reset_time;
		int curve_volume;
		int curve_pitch;
		int curve_duty;
		int curve_pan;
		float dutycycle;
		int octave;
		int note_length;
		float volume;
		int tempo;
		int note;
		int volume_section;
		int volume_offset_section;
		int dutycycle_section;
		int pan_section;
		int pos;
		std::string tok;
		int length_in_samples;
		int note_fulfilled;
		bool done;
		bool padded;
		bool loop;
		bool playing;
		int t;
		float last_freq;
		float last_start;
		int same_sections;
		float last_freq_o;
		float last_start_o;
		int same_sections_o;
		float master_volume;
		float mix_volume;
		float last_noise;
		float last_noise2;
		float remain;
		bool fading;
		float prev_time;
		std::vector<Sample *> wav_samples;
		std::vector<Wav_Start> wav_starts; // <sample index, sample to start at>
		int wav_sample;

		math::Interpolator *freq_interp;
		math::Interpolator *freq_interp_o;
		int prev_note;
		int prev_note_o;
		int prev_section;
		int prev_section_o;
		math::Interpolator *vol_interp;
		math::Interpolator *vol_interp_o;
		math::Interpolator *duty_interp;
		math::Interpolator *pan_interp;

		MML *mml;

		Uint32 beginning_silence;

		float internal_volume;

		std::vector<Reverb_Type> reverb_types;

		int buzz_freq;
		Wave_Type buzz_type;
		float buzz_volume;

		int abs_sample;

		bool no_fade;

		bool ignore_tempo_changes;

		int new_tempo;

		util::Callback finished_callback;
	       	void *finished_callback_data;
		bool _call_callbacks;

		float pan;
		float global_pan;
	};

	SHIM5_EXPORT void load(SDL_IOStream *f, bool load_from_filesystem);

	std::vector<Track *> tracks;
	std::vector<Track *> reverb_tracks;

	std::vector<Sample *> wav_samples;

	static std::vector<MML *> loaded_mml;

	std::string name;

	Uint32 instance;

	MML_Data *mml_data;
};

} // End namespace audio

} // End namespace noo

#endif // NOO_MML_H
