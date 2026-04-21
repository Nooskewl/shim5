#ifndef NOO_SAMPLE_H
#define NOO_SAMPLE_H

namespace noo {

namespace audio {

class Sample;

typedef Uint8 *(*sample_loader)(SDL_IOStream *file, char *errmsg, SDL_AudioSpec *spec, Uint32 *size);

Uint8 *decode_wav(SDL_IOStream *file, char *errmsg, SDL_AudioSpec *spec, Uint32 *size);

struct Sample_Instance {
	SDL_AudioSpec *spec;
	Uint8 *data;
	Uint32 length;
	Uint32 play_length;
	Uint32 offset;
	Uint32 silence;
	bool loop;
	float volume;
	Sample *sample;
	int bits_per_sample;
	int bytes_per_sample;
	bool format_is_float;
	bool format_is_signed;
	bool format_should_be_swapped;
	float min_sample;
	float max_sample;
	bool paused;
	util::Callback finished_callback;
	void *finished_callback_data;
};

class SHIM5_EXPORT Sample {
public:
	static void stop_instance(Sample_Instance *s);
	static void set_instance_volume(Sample_Instance *s, float volume);
	static void pause_instance(Sample_Instance *s, bool onoff);
	static bool sample_active(Sample_Instance *s);

	SHIM5_EXPORT static void register_sample_loader(std::string ext, sample_loader func);

	Sample(std::string filename, bool load_from_filesystem = false);
	// S16LE constructor
	Sample(Uint8 *data, int size, int freq, int channels);
	virtual ~Sample();

	static void update();

	void play(float volume, bool loop);
	void play(bool loop);
	bool is_done();
	void stop(); // this does a stop_all
	bool is_playing();

	// Play length/silence is in samples based on the device frequency (audio::internal::audio_context.device_spec.freq)
	// If play_length is 0, it plays unstretched
	// silence is samples until it starts
	Sample_Instance *play_stretched(float volume, Uint32 silence, Uint32 play_length, bool loop, util::Callback finished_callback = 0, void *finished_callback_data = 0);

	void stop_all();

	void set_done(bool done);

	Uint32 get_length();
	int get_frequency();

private:
	void delete_instances();

	SDL_IOStream *file;
	SDL_AudioSpec *spec;
	Uint8 *data;
	Uint32 length;
	bool done;
};

} // End namespace audio

} // End namespace noo

#endif // NOO_SAMPLE_H
