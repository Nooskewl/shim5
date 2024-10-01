#ifndef NOO_ACHIEVEMENTS_H
#define NOO_ACHIEVEMENTS_H

#include "shim5/main.h"

namespace noo {

namespace util {

bool SHIM5_EXPORT achieve(void *id);
bool SHIM5_EXPORT show_achievements();

// FIXME: stuff for setting up achievements (store system-specific ids/etc)

} // End namespace util

} // End namespace noo

#endif // NOO_ACHIEVEMENTS_H
