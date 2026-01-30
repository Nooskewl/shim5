#ifndef NOO_I_UTIL_H
#define NOO_I_UTIL_H

namespace noo {

namespace util {

namespace internal {

void SHIM5_EXPORT print_string(int level, const char *string);
std::string SHIM5_EXPORT get_game_name();

#ifdef _WIN32
/* MSVC doesn't have snprintf */

#define snprintf noo::util::internal::c99_snprintf

SHIM5_EXPORT int c99_vsnprintf(char* str, int size, const char* format, va_list ap);
SHIM5_EXPORT int c99_snprintf(char* str, int size, const char* format, ...);
#endif // _WIN32

void close_log_file();
void flush_log_file();

} // End namespace internal

} // End namespace util

} // End namespace noo

#endif // NOO_I_UTIL_H
