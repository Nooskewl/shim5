#ifndef NOO_LIBUTIL_H
#define NOO_LIBUTIL_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <glob.h>
#endif

#include <shim4/main.h>

namespace noo {

namespace util {

SHIM4_EXPORT void mkdir(std::string path);

SHIM4_EXPORT std::string get_system_language(); // returns language in Steam format like "english", "french" etc

class SHIM4_EXPORT List_Directory {
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

SHIM4_EXPORT std::string uppercase(std::string);
SHIM4_EXPORT std::string lowercase(std::string);

// For trimming whitespace from left, right or both
SHIM4_EXPORT std::string &ltrim(std::string &s);
SHIM4_EXPORT std::string &rtrim(std::string &s);
SHIM4_EXPORT std::string &trim(std::string &s);

SHIM4_EXPORT std::string unescape_string(std::string);

SHIM4_EXPORT std::string load_text_from_filesystem(std::string filename);

SHIM4_EXPORT std::string remove_quotes(std::string s);

SHIM4_EXPORT int utf8_len(std::string text);
SHIM4_EXPORT int utf8_len_bytes(std::string text, int char_count);
SHIM4_EXPORT uint32_t utf8_char_next(std::string text, int &offset);
SHIM4_EXPORT uint32_t utf8_char_offset(std::string text, int o);
SHIM4_EXPORT uint32_t utf8_char(std::string text, int i);
SHIM4_EXPORT std::string utf8_char_to_string(uint32_t ch);
SHIM4_EXPORT std::string utf8_substr(std::string s, int start, int count = -1);

} // End namespace util

} // End namespace noo

#endif // NOO_LIBUTIL_H
