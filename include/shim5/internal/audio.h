#ifndef NOO_I_AUDIO_H
#define NOO_I_AUDIO_H

namespace noo {

namespace audio {

namespace internal {

struct Audio_Context {
	bool mute;
	SDL_AudioSpec device_spec;
	std::vector<Sample_Instance *> playing_samples;
};

extern SHIM5_EXPORT Audio_Context audio_context;

extern std::vector<util::Callback> audio_callbacks;
extern std::vector<void *> audio_callback_data;

} // End namespace internal

} // End namespace audio

} // End namespace noo

#endif // NOO_I_AUDIO_H
