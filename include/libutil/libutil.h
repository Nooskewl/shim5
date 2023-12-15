#ifndef NOO_LIBUTIL_H
#define NOO_LIBUTIL_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <glob.h>
#endif

namespace noo {

namespace util {

void mkdir(std::string path);

std::string get_system_language(); // returns language in Steam format like "english", "french" etc
class List_Directory {
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

std::string uppercase(std::string);
std::string lowercase(std::string);

// For trimming whitespace from left, right or both
std::string &ltrim(std::string &s);
std::string &rtrim(std::string &s);
std::string &trim(std::string &s);

std::string unescape_string(std::string);

std::string load_text_from_filesystem(std::string filename);

} // End namespace util

} // End namespace noo

#endif // NOO_LIBUTIL_H
