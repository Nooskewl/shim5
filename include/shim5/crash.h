#ifndef NOO_CRASH_H
#define NOO_CRASH_H

#include "shim5/main.h"

using namespace noo;

namespace noo {

namespace util {

void SHIM5_EXPORT start_crashdumps();
void SHIM5_EXPORT end_crashdumps();

} // End namespace util

} // End namespace noo

#endif // NOO_CRASH_H
