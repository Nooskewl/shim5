// WARNING: any of the 'printf' style functions in this file have limits on supported sizes of strings

#ifndef NOO_UTIL_H
#define NOO_UTIL_H

#include "shim5/main.h"

#include "libutil/libutil.h"

namespace noo {

namespace util {

bool basic_start();
bool static_start();
void static_end();

bool start();
void end();

template <typename T> T sign(T v) { return (T(0) < v) - (v < T(0)); }

void SHIM5_EXPORT errormsg(const char *fmt, ...);
void SHIM5_EXPORT errormsg(std::string s);
void SHIM5_EXPORT infomsg(const char *fmt, ...);
void SHIM5_EXPORT infomsg(std::string s);
void SHIM5_EXPORT debugmsg(const char *fmt, ...);
void SHIM5_EXPORT debugmsg(std::string s);
void SHIM5_EXPORT verbosemsg(const char *fmt, ...);
void SHIM5_EXPORT verbosemsg(std::string s);
void SHIM5_EXPORT printGLerror(const char *fmt, ...);
#ifdef DEBUG
#define PRINT_GL_ERROR(...) util::printGLerror(__VA_ARGS__)
#else
#define PRINT_GL_ERROR(...)
#endif

// some functions SDL doesn't have that are handy
int SHIM5_EXPORT SDL_fgetc(SDL_IOStream *file);
int SHIM5_EXPORT SDL_fputc(int c, SDL_IOStream *file);
char SHIM5_EXPORT *SDL_fgets(SDL_IOStream *file, char * const buf, size_t max);
int SHIM5_EXPORT SDL_fputs(const char *string, SDL_IOStream *file);
SHIM5_EXPORT void SDL_fprintf(SDL_IOStream *file, const char *fmt, ...);

SDL_IOStream *open_file(std::string filename, int *sz, bool data_only = false);
void close_file(SDL_IOStream *file);
void free_data(SDL_IOStream *file);

int SHIM5_EXPORT check_args(int argc, char **argv, std::string arg);
bool SHIM5_EXPORT bool_arg(bool default_value, int argc, char **argv, std::string arg);

SHIM5_EXPORT std::string string_printf(const char *fmt, ...);

std::string SHIM5_EXPORT escape_string(std::string s, char c); // add backslashes before c characters in s

std::string SHIM5_EXPORT load_text(std::string filename);
char SHIM5_EXPORT *slurp_file(std::string filename, int *sz);
char SHIM5_EXPORT *slurp_file_from_filesystem(std::string filename, int *sz);

void SHIM5_EXPORT open_with_system(std::string filename); // open with default app
void SHIM5_EXPORT open_url(std::string url);

#ifdef ANDROID
bool SHIM5_EXPORT is_chromebook();
#endif

Uint64 file_date(std::string filename);

#ifndef _WIN32 // FIXME: need this for Windows
time_t utc_secs();
#endif

} // End namespace util

} // End namespace noo

#endif // NOO_UTIL_H
