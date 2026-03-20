#ifndef NOO_SAMPLE_H
#define NOO_SAMPLE_H

namespace noo {

namespace audio {

class Sample;

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
};

class SHIM5_EXPORT Sample {
public:
	static void stop_instance(Sample_Instance *s);
	static void set_instance_volume(Sample_Instance *s, float volume);
	static void pause_instance(Sample_Instance *s, bool onoff);
	static bool sample_active(Sample_Instance *s);

	Sample(std::string filename, bool load_from_filesystem = false);
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
	Sample_Instance *play_stretched(float volume, Uint32 silence, Uint32 play_length, bool loop);

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

	bool do_free;
};

} // End namespace audio

} // End namespace noo

#endif // NOO_SAMPLE_H
