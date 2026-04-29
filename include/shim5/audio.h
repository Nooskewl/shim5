#ifndef NOO_AUDIO_H
#define NOO_AUDIO_H

#include "shim5/main.h"

#define SHIM_AUDIO_BUFFER_SIZE 256*1024

namespace noo {

namespace audio {

bool SHIM5_EXPORT static_start();
bool SHIM5_EXPORT start();
void SHIM5_EXPORT end();
int SHIM5_EXPORT millis_to_samples(int millis, int freq = -1);
int SHIM5_EXPORT samples_to_millis(int samples, int freq = -1);
float SHIM5_EXPORT calc_pan_left(float pan);
float SHIM5_EXPORT calc_pan_right(float pan);
void SHIM5_EXPORT stop_all_samples();
void SHIM5_EXPORT lock_mutex();
void SHIM5_EXPORT unlock_mutex();

} // End namespace audio

} // End namespace noo

#endif // NOO_AUDIO_H
