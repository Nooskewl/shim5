#include "shim5/cpa.h"
#include "shim5/crash.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/vertex_cache.h"
#include "libutil/libutil.h"

#ifdef SDL_PLATFORM_APPLE
#include "shim5/apple.h"
#ifdef IOS
#include "shim5/ios.h"
#else
#include "shim5/macosx.h"
#endif
#endif

#include <sys/stat.h>

#ifdef _WIN32
#include <shlobj.h>
#include <dbghelp.h>
#else
#include <sys/types.h>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

#ifdef ANDROID
#include <jni.h>
#endif

#ifdef STEAMWORKS
#include "shim5/steamworks.h"
#endif

#include "shim5/internal/gfx.h"
#include "shim5/internal/util.h"

using namespace noo;

static FILE *log_file;

#ifdef _WIN32
static void print_string_console(const char *string)
{
	OutputDebugString(string);
	printf("%s", string);
}
#elif defined SDL_PLATFORM_APPLE && !defined IOS
static void print_string_console(const char *string)
{
	noo::util::macosx_log(string);
}
#elif defined ANDROID
static void print_string_console(const char *string)
{
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(string);

	jmethodID method_id = env->GetMethodID(clazz, "logString", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}
#else
static void print_string_console(const char *string)
{
	printf("%s", string);
}
#endif

static void print_string(int level, const char *string)
{
	if (level <= 3 && log_file) { // Don't do verbose logging to log file
		fprintf(log_file, "%s", string);
	}
	if (level <= shim::error_level) {
		print_string_console(string);
	}
}

namespace noo {

namespace util {

bool basic_start()
{
	start_crashdumps();
	
	log_file = 0;

	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+error-level")) > 0) {
		shim::error_level = atoi(shim::argv[index+1]);
	}

	return true;
}

bool static_start()
{
#ifndef IOS
	if (shim::logging) {
#ifdef ANDROID
		std::string log_filename = util::get_standard_path(util::SAVED_GAMES, true) + "/log.txt";
#else
		std::string log_filename = get_appdata_dir() + "/log.txt";
#endif
		log_file = fopen(log_filename.c_str(), "w");
	}
#endif

	return true;
}

void static_end()
{
	internal::close_log_file();

	end_crashdumps();
}

bool start()
{
	srand((uint32_t)time(0));

	return true;
}

void end()
{
}

static std::string get_game_name()
{
	if (shim::game_name != "") {
		return shim::game_name;
	}
	return "Nooskewl Shim";
}

void errormsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (ERROR): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(1, buf);
	va_end(v);
	delete[] fmt2;
}

void errormsg(std::string s)
{
	errormsg(s.c_str());
}

void infomsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (INFO): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(2, buf);
	va_end(v);
	delete[] fmt2;
}

void infomsg(std::string s)
{
	infomsg(s.c_str());
}

void debugmsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (DEBUG): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(3, buf);
	va_end(v);
	delete[] fmt2;
}

void debugmsg(std::string s)
{
	debugmsg(s.c_str());
}

void verbosemsg(const char *fmt, ...)
{
	std::string prefix = get_game_name();
	int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
	va_list v;
	char *fmt2 = new char[len];
	char buf[5000];
	if (shim::log_tags) {
		snprintf(fmt2, len, "%s (VERBOSE): ", prefix.c_str());
	}
	else {
		fmt2[0] = 0;
	}
	strcat(fmt2, fmt);
	va_start(v, fmt);
	vsnprintf(buf, sizeof(buf), fmt2, v);
	print_string(4, buf);
	va_end(v);
	delete[] fmt2;
}

void verbosemsg(std::string s)
{
	verbosemsg(s.c_str());
}

void printGLerror(const char *fmt, ...)
{
#ifdef DEBUG
	if (true) {
#else
	if (shim::debug) {
#endif
		GLenum error;
		if ((error = glGetError_ptr()) == GL_NO_ERROR) {
			return;
		}
		std::string prefix = get_game_name();
		int len = int(strlen(prefix.c_str()) + 50 + strlen(fmt));
		va_list v;
		char *fmt2 = new char[len];
		char buf[5000];
		if (shim::log_tags) {
			snprintf(fmt2, len, "%s (OPENGL:%d): ", prefix.c_str(), error);
		}
		else {
			fmt2[0] = 0;
		}
		strcat(fmt2, fmt);
		strcat(fmt2, ".\n");
		va_start(v, fmt);
		vsnprintf(buf, sizeof(buf), fmt2, v);
		print_string(3, buf);
		va_end(v);
		delete[] fmt2;
	}
}

int SDL_fgetc(SDL_IOStream *file)
{
	unsigned char c;
	if (SDL_ReadIO(file, &c, 1) == 0) {
		return EOF;
	}
	return c;
}

int SDL_fputc(int c, SDL_IOStream *file)
{
	return SDL_WriteIO(file, &c, 1) == 1 ? 1 : EOF;
}

char *SDL_fgets(SDL_IOStream *file, char * const buf, size_t max)
{
	size_t c = 0;
	while (c < max) {
		int i = SDL_fgetc(file);
		if (i == -1) {
			break;
		}
		buf[c] = (char)i;
		c++;
		if (i == '\n') {
			break;
		}
	}
	if (c == 0) return 0;
	buf[c] = 0;
	return buf;
}

int SDL_fputs(const char *string, SDL_IOStream *file)
{
	size_t len = strlen(string);
	return SDL_WriteIO(file, string, len) < len ? EOF : 0;
}

void SDL_fprintf(SDL_IOStream *file, const char *fmt, ...)
{
	char buf[1000];
	va_list v;
	va_start(v, fmt);
	vsnprintf(buf, 1000, fmt, v);
	va_end(v);

	SDL_fputs(buf, file);
}

SDL_IOStream *open_file(std::string filename, int *sz, bool data_only)
{
	SDL_IOStream *file;
	if (shim::cpa) {
		file = shim::cpa->open(filename, sz, data_only);
	}
	else {
		const char *base = SDL_GetBasePath();
		filename = std::string(base) + "data/" + filename;
		file = SDL_IOFromFile(filename.c_str(), "rb");
		if (file && sz) {
			*sz = (int)SDL_GetIOSize(file);
			if (data_only) {
				Uint8 *buf = new Uint8[*sz];
				int count = 0;
				const int chunk_size = 32768;
				while (true) {
					int read;
					int to_read = MIN(*sz-count, chunk_size);
					if ((read = (int)SDL_ReadIO(file, buf+count, to_read)) < to_read) {
						break;
					}
					count += read;
					if (count == *sz) {
						break;
					}
				}
				SDL_CloseIO(file);
				return (SDL_IOStream *)buf;
			}
		}
	}
	if (file == 0) {
		throw FileNotFoundError(filename);
	}
	return file;
}

void close_file(SDL_IOStream *file)
{
	if (shim::cpa) {
		shim::cpa->close(file);
	}
	else {
		SDL_CloseIO(file);
	}
}

void free_data(SDL_IOStream *file)
{
	if (shim::cpa) {
		shim::cpa->free_data(file);
	}
	// else, do nothing
}

std::string string_printf(const char *fmt, ...)
{
	char buf[1000];
	va_list v;
	va_start(v, fmt);
	vsnprintf(buf, 1000, fmt, v);
	va_end(v);

	return buf;
}

std::string escape_string(std::string s, char c)
{
	std::string ret;

	for (int i = 0; i < (int)s.length(); i++) {
		if (s[i] == c) {
			ret += "\\";
		}
		ret += s.substr(i, 1);
	}

	return ret;
}

std::string unescape_string(std::string);

std::string load_text(std::string filename)
{
	int size;
	SDL_IOStream *file = open_file(filename, &size);

	char *buf = new char[size+1];

	if (SDL_ReadIO(file, buf, size) != size) {
		close_file(file);
		throw LoadError(filename);
	}

	buf[size] = 0;

	std::string s = buf;

	delete[] buf;

	close_file(file);

	return s;
}

char *slurp_file(std::string filename, int *sz)
{
	int _sz;
	SDL_IOStream *file = open_file(filename, &_sz);

	char *buf = new char[_sz];

	if (SDL_ReadIO(file, buf, _sz) != _sz) {
		close_file(file);
		throw LoadError(filename);
	}

	close_file(file);

	if (sz) {
		*sz = _sz;
	}

	return buf;
}

char *slurp_file_from_filesystem(std::string filename, int *sz)
{
	SDL_IOStream *file = SDL_IOFromFile(filename.c_str(), "rb");

	if (file == 0) {
		throw FileNotFoundError(filename);
	}

	int _sz = (int)SDL_GetIOSize(file);

	char *buf = new char[_sz];

	if (SDL_ReadIO(file, buf, _sz) != _sz) {
		throw LoadError(filename);
	}

	SDL_CloseIO(file);

	if (sz) {
		*sz = _sz;
	}

	return buf;
}

void open_with_system(std::string filename)
{
#ifdef _WIN32
	if (gfx::internal::gfx_context.fullscreen) {
		ShowWindow(gfx::internal::gfx_context.hwnd, SW_MINIMIZE);
	}
	ShellExecute(0, 0, filename.c_str(), 0, 0 , SW_SHOW);
#elif defined __linux__
	pid_t pid = fork();
	if (pid == 0) {
		system((std::string("xdg-open ") + filename).c_str());
		exit(0);
	}
#elif !defined IOS && !defined __EMSCRIPTEN__
	macosx_open_with_system(filename);
#endif
}

void open_url(std::string url)
{
#ifdef ANDROID
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(url.c_str());

	jmethodID method_id = env->GetMethodID(clazz, "openURL", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
#elif defined __linux__
	// FIXME: might work on win/mac too
	open_with_system(url);
#endif
}

Uint64 file_date(std::string filename)
{
#ifdef TVOS
	return tvos_file_date(filename);
#else
	struct stat s;
	if (stat(filename.c_str(), &s) == 0) {
		return s.st_ctime;
	}
	else {
		return -1;
	}
#endif
}

#ifndef _WIN32 // FIXME: implement for Windows
time_t utc_secs()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec;
}
#endif

namespace internal {

#ifdef _WIN32
int c99_vsnprintf(char* str, int size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

int c99_snprintf(char* str, int size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}
#endif // _WIN32

void close_log_file()
{
	if (log_file) {
		fclose(log_file);
		log_file = 0;
	}
}

void flush_log_file()
{
	fflush(log_file);
}

} // End namespace internal

} // End namespace util

} // End namespace noo
