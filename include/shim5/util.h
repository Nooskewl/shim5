// WARNING: any of the 'printf' style functions in this file have limits on supported sizes of strings

#ifndef NOO_UTIL_H
#define NOO_UTIL_H

#include "shim5/main.h"

#include "libutil/libutil.h"

namespace noo {

namespace util {

void SHIM5_EXPORT printGLerror(const char *fmt, ...);
#ifdef DEBUG
#define PRINT_GL_ERROR(...) util::printGLerror(__VA_ARGS__)
#else
#define PRINT_GL_ERROR(...)
#endif
} // End namespace util

} // End namespace noo

void SHIM5_EXPORT open_with_system(std::string filename); // open with default app
void SHIM5_EXPORT open_url(std::string url);

#endif // NOO_UTIL_H
