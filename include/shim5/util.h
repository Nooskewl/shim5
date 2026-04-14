// WARNING: any of the 'printf' style functions in this file have limits on supported sizes of strings

#ifndef NOO_UTIL_H
#define NOO_UTIL_H

#include "shim5/main.h"

namespace noo {

namespace util {

class CPA;
class JSON;

class SHIM5_EXPORT Error {
public:
	SHIM5_EXPORT Error();
	SHIM5_EXPORT Error(std::string error_message);
	SHIM5_EXPORT virtual ~Error();
	
	std::string error_message;
};

class SHIM5_EXPORT MemoryError : public Error {
public:
	SHIM5_EXPORT MemoryError(std::string error_message);
	SHIM5_EXPORT virtual ~MemoryError();
};

class SHIM5_EXPORT LoadError : public Error {
public:
	SHIM5_EXPORT LoadError(std::string error_message);
	SHIM5_EXPORT virtual ~LoadError();
};

class SHIM5_EXPORT FileNotFoundError : public Error {
public:
	SHIM5_EXPORT FileNotFoundError(std::string error_message);
	SHIM5_EXPORT virtual ~FileNotFoundError();
};

class SHIM5_EXPORT ParseError : public Error {
public:
	SHIM5_EXPORT ParseError(std::string error_message);
	SHIM5_EXPORT virtual ~ParseError();
};

class SHIM5_EXPORT GLError : public Error {
public:
	SHIM5_EXPORT GLError(std::string error_message);
	SHIM5_EXPORT virtual ~GLError();
};

enum Path_Type {
	DOCUMENTS = 1,
	APPDATA,
	HOME,
	SAVED_GAMES
};

void SHIM5_EXPORT mkdir(std::string path);

std::string SHIM5_EXPORT get_system_language(); // returns language in Steam format like "english", "french" etc

class SHIM5_EXPORT List_Directory {
public:
	List_Directory(std::string filespec);
	~List_Directory();

	std::string next();

private:
	bool got_first;
	bool done;
	HANDLE handle;
	WIN32_FIND_DATAW ffd;
};

std::string SHIM5_EXPORT uppercase(std::string);
std::string SHIM5_EXPORT lowercase(std::string);

// For trimming whitespace from left, right or both
std::string SHIM5_EXPORT &ltrim(std::string &s);
std::string SHIM5_EXPORT &rtrim(std::string &s);
std::string SHIM5_EXPORT &trim(std::string &s);

std::string SHIM5_EXPORT unescape_string(std::string);

std::string SHIM5_EXPORT load_text_from_filesystem(std::string filename);

std::string SHIM5_EXPORT remove_quotes(std::string s);

std::string SHIM5_EXPORT itos(int i);

int SHIM5_EXPORT utf8_len(std::string text);
int SHIM5_EXPORT utf8_len_bytes(std::string text, int char_count);
uint32_t SHIM5_EXPORT utf8_char_next(std::string text, int &offset);
uint32_t SHIM5_EXPORT utf8_char_offset(std::string text, int o);
uint32_t SHIM5_EXPORT utf8_char(std::string text, int i);
std::string SHIM5_EXPORT utf8_char_to_string(uint32_t ch);
std::string SHIM5_EXPORT utf8_substr(std::string s, int start, int count = -1);

void SHIM5_EXPORT srand(uint32_t s);
uint32_t SHIM5_EXPORT rand();
uint32_t SHIM5_EXPORT rand(int min, int max_inclusive);

// These 3 are safe to call before calling shim::start
std::string SHIM5_EXPORT get_standard_path(Path_Type type, bool create);
// appdata_dir is used for crashdumps, can be used for anything else you want like config files
std::string SHIM5_EXPORT get_appdata_dir();
std::string SHIM5_EXPORT get_savegames_dir();
void SHIM5_EXPORT set_appdata_dir(std::string appdata_dir, bool create);

class SHIM5_EXPORT Tokenizer {
public:

	Tokenizer(std::string s, char delimiter, bool skip_bunches = false);
	std::string next();
	std::string remaining();

private:
	std::string s;
	char delimiter;
	size_t offset;
	bool skip_bunches;
};

int SHIM5_EXPORT check_args(int argc, char **argv, std::string arg);
bool SHIM5_EXPORT bool_arg(bool default_value, int argc, char **argv, std::string arg);

bool SHIM5_EXPORT basic_start();
bool SHIM5_EXPORT static_start();
void SHIM5_EXPORT static_end();

bool SHIM5_EXPORT start();
void SHIM5_EXPORT end();

template <typename T> T sign(T v) { return (T(0) < v) - (v < T(0)); }

void SHIM5_EXPORT errormsg(const char *fmt, ...);
void SHIM5_EXPORT errormsg(std::string s);
void SHIM5_EXPORT infomsg(const char *fmt, ...);
void SHIM5_EXPORT infomsg(std::string s);
void SHIM5_EXPORT debugmsg(const char *fmt, ...);
void SHIM5_EXPORT debugmsg(std::string s);
void SHIM5_EXPORT verbosemsg(const char *fmt, ...);
void SHIM5_EXPORT verbosemsg(std::string s);

// some functions SDL doesn't have that are handy
int SHIM5_EXPORT SDL_fgetc(SDL_IOStream *file);
int SHIM5_EXPORT SDL_fputc(int c, SDL_IOStream *file);
char SHIM5_EXPORT *SDL_fgets(SDL_IOStream *file, char * const buf, size_t max);
int SHIM5_EXPORT SDL_fputs(const char *string, SDL_IOStream *file);
void SHIM5_EXPORT SDL_fprintf(SDL_IOStream *file, const char *fmt, ...);

SDL_IOStream SHIM5_EXPORT *open_file(std::string filename, int *sz, bool data_only = false);
void SHIM5_EXPORT close_file(SDL_IOStream *file);
void SHIM5_EXPORT free_data(SDL_IOStream *file);

std::string SHIM5_EXPORT string_printf(const char *fmt, ...);

std::string SHIM5_EXPORT escape_string(std::string s, char c); // add backslashes before c characters in s

std::string SHIM5_EXPORT load_text(std::string filename);
char SHIM5_EXPORT *slurp_file(std::string filename, int *sz);
char SHIM5_EXPORT *slurp_file_from_filesystem(std::string filename, int *sz);

Uint64 SHIM5_EXPORT file_date(std::string filename);

time_t SHIM5_EXPORT utc_secs();

void SHIM5_EXPORT printGLerror(const char *fmt, ...);
#ifdef DEBUG
#define PRINT_GL_ERROR(...) util::printGLerror(__VA_ARGS__)
#else
#define PRINT_GL_ERROR(...)
#endif

void SHIM5_EXPORT open_with_system(std::string filename); // open with default app
void SHIM5_EXPORT open_url(std::string url);

} // End namespace util

} // End namespace noo

#endif // NOO_UTIL_H
