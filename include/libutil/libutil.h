#ifndef NOO_LIBUTIL_H
#define NOO_LIBUTIL_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <glob.h>
#endif

#include <shim5/main.h>

namespace noo {

namespace util {

class Error {
public:
	SHIM5_EXPORT Error();
	SHIM5_EXPORT Error(std::string error_message);
	SHIM5_EXPORT virtual ~Error();
	
	std::string error_message;
};

class MemoryError : public Error {
public:
	SHIM5_EXPORT MemoryError(std::string error_message);
	SHIM5_EXPORT virtual ~MemoryError();
};

class LoadError : public Error {
public:
	SHIM5_EXPORT LoadError(std::string error_message);
	SHIM5_EXPORT virtual ~LoadError();
};

class FileNotFoundError : public Error {
public:
	SHIM5_EXPORT FileNotFoundError(std::string error_message);
	SHIM5_EXPORT virtual ~FileNotFoundError();
};

class ParseError : public Error {
public:
	SHIM5_EXPORT ParseError(std::string error_message);
	SHIM5_EXPORT virtual ~ParseError();
};

class GLError : public Error {
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

SHIM5_EXPORT void mkdir(std::string path);

SHIM5_EXPORT std::string get_system_language(); // returns language in Steam format like "english", "french" etc

class SHIM5_EXPORT List_Directory {
public:
	List_Directory(std::string filespec);
	~List_Directory();

	std::string next();

private:
#ifdef _WIN32
	bool got_first;
	bool done;
	HANDLE handle;
	WIN32_FIND_DATA ffd;
#elif !defined ANDROID
	int i;
	glob_t gl;
#endif
};

SHIM5_EXPORT std::string uppercase(std::string);
SHIM5_EXPORT std::string lowercase(std::string);

// For trimming whitespace from left, right or both
SHIM5_EXPORT std::string &ltrim(std::string &s);
SHIM5_EXPORT std::string &rtrim(std::string &s);
SHIM5_EXPORT std::string &trim(std::string &s);

SHIM5_EXPORT std::string unescape_string(std::string);

SHIM5_EXPORT std::string load_text_from_filesystem(std::string filename);

SHIM5_EXPORT std::string remove_quotes(std::string s);

SHIM5_EXPORT std::string itos(int i);

SHIM5_EXPORT int utf8_len(std::string text);
SHIM5_EXPORT int utf8_len_bytes(std::string text, int char_count);
SHIM5_EXPORT uint32_t utf8_char_next(std::string text, int &offset);
SHIM5_EXPORT uint32_t utf8_char_offset(std::string text, int o);
SHIM5_EXPORT uint32_t utf8_char(std::string text, int i);
SHIM5_EXPORT std::string utf8_char_to_string(uint32_t ch);
SHIM5_EXPORT std::string utf8_substr(std::string s, int start, int count = -1);

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

private:
	std::string s;
	char delimiter;
	size_t offset;
	bool skip_bunches;
};

} // End namespace util

} // End namespace noo

namespace noo {
namespace shim {

extern SHIM5_EXPORT std::string organisation_name; // set this first thing too
extern SHIM5_EXPORT std::string game_name; // set this first thing too

} // End namespace shim
} // End namespace noo

#endif // NOO_LIBUTIL_H
