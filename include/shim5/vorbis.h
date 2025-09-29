#ifndef NOO_VORBIS_H
#define NOO_VORBIS_H

#include "shim5/main.h"

namespace noo {

namespace audio {

Uint8 *decode_vorbis(SDL_IOStream *file, char *errmsg, SDL_AudioSpec *spec, Uint32 *size); // Returns 0 on error, fills errmsg

} // End namespace audio

} // End namespace noo

#endif // NOO_VORBIS_H
