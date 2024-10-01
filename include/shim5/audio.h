#ifndef NOO_AUDIO_H
#define NOO_AUDIO_H

#include "shim5/main.h"

namespace noo {

namespace audio {

bool SHIM5_EXPORT static_start();
bool SHIM5_EXPORT start();
void SHIM5_EXPORT end();
int SHIM5_EXPORT millis_to_samples(int millis);
int SHIM5_EXPORT samples_to_millis(int samples, int freq = -1);
void SHIM5_EXPORT pause_sfx(bool paused);

} // End namespace audio

} // End namespace noo

#endif // NOO_AUDIO_H
