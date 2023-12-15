#include <string>
#include <fstream>

#include "libutil/libutil.h"

#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
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
#include "shim4/steamworks.h"
#endif

namespace noo {

namespace util {

void mkdir(std::string path)
{
#ifdef _WIN32
	_mkdir(path.c_str());
#elif defined IOS
	ios_mkdir(path);
#else
	::mkdir(path.c_str(), 0700);
#endif
}

#ifdef _WIN32
std::string get_system_language_windows()
{
	LONG l = GetUserDefaultLCID();

	// returns names in Steam format

	if (
		l == 1031 ||
		l == 2055 ||
		l == 3079 ||
		l == 4103 ||
		l == 5127
	) {
		return "german";
	}
	else if (l == 1032) {
		return "greek";
	}
	else if (
		l == 1034 ||
		l == 2058 ||
		l == 3082 ||
		l == 4106 ||
		l == 5130 ||
		l == 6154 ||
		l == 7178 ||
		l == 8202 ||
		l == 9226 ||
		l == 10250 ||
		l == 11274 ||
		l == 12298 ||
		l == 13322 ||
		l == 14346 ||
		l == 15370 ||
		l == 16394 ||
		l == 17418 ||
		l == 18442 ||
		l == 19466 ||
		l == 20490
	) {
		return "spanish";
	}
	else if (
		l == 1036 ||
		l == 2060 ||
		l == 3084 ||
		l == 4108 ||
		l == 5132
	) {
		return "french";
	}
	else if (
		l == 1043 ||
		l == 2067
	) {
		return "dutch";
	}
	else if (l == 1045) {
		return "polish";
	}
	else if (l == 1046) {
		return "brazilian";
	}
	else if (l == 2070) {
		return "portuguese";
	}
	else if (l == 1049) {
		return "russian";
	}
	else if (l == 1042) {
		return "korean";
	}
	else {
		return "english";
	}
}
#endif

#if defined __linux__ && !defined ANDROID
#include <langinfo.h>

std::string get_system_language_linux()
{
	std::string str;
	if (getenv("LANG")) {
		str = getenv("LANG");
	}
	else {
		str = nl_langinfo(_NL_IDENTIFICATION_LANGUAGE);
	}

	std::string l_str = str.substr(0, 5);
	str = str.substr(0, 2);

	// convert to steam style since that was the first one we did
	if (str == "de") {
		str = "german";
	}
	else if (str == "fr") {
		str = "french";
	}
	else if (str == "nl") {
		str = "dutch";
	}
	else if (str == "el") {
		str = "greek";
	}
	else if (str == "it") {
		str = "italian";
	}
	else if (str == "pl") {
		str = "polish";
	}
	else if (str == "pt") {
		if (l_str == "pt_BR") {
			str = "brazilian";
		}
		else {
			str = "portuguese";
		}
	}
	else if (str == "ru") {
		str = "russian";
	}
	else if (str == "es") {
		str = "spanish";
	}
	else if (str == "ko") {
		str = "korean";
	}
	else {
		str = "english";
	}

	return str;
}
#endif

#ifdef ANDROID
std::string get_system_language_android()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "get_android_language", "()Ljava/lang/String;");

	jstring s = (jstring)env->CallObjectMethod(activity, method_id);

	const char *native = env->GetStringUTFChars(s, 0);

	std::string lang = native;

	env->ReleaseStringUTFChars(s, native);

	env->DeleteLocalRef(s);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	std::string l_str = lang.substr(0, 5);
	std::string str = lang.substr(0, 2);

	// convert to steam style since that was the first one we did
	if (str == "de") {
		str = "german";
	}
	else if (str == "fr") {
		str = "french";
	}
	else if (str == "nl") {
		str = "dutch";
	}
	else if (str == "el") {
		str = "greek";
	}
	else if (str == "it") {
		str = "italian";
	}
	else if (str == "pl") {
		str = "polish";
	}
	else if (str == "pt") {
		if (l_str == "pt-BR") {
			str = "brazilian";
		}
		else {
			str = "portuguese";
		}
	}
	else if (str == "ru") {
		str = "russian";
	}
	else if (str == "es") {
		str = "spanish";
	}
	else if (str == "ko") {
		str = "korean";
	}
	else {
		str = "english";
	}

	return str;
}

bool is_chromebook()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "is_chromebook", "()Z");

	bool result = (bool)env->CallBooleanMethod(activity, method_id);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return result;
}
#endif

std::string get_system_language()
{
#ifdef STEAMWORKS
	if (shim::steam_init_failed == false) {
		return get_steam_language();
	}
#endif
#ifdef _WIN32
	return get_system_language_windows();
#elif defined __linux__ && !defined ANDROID
	return get_system_language_linux();
#elif defined ANDROID
	return get_system_language_android();
#else
	return apple_get_system_language();
#endif
}

std::string &ltrim(std::string &s)
{
	int i = 0;
	while (i < (int)s.length() && isspace(s[i])) {
		i++;
	}
	if (i >= (int)s.length()) {
		s = "";
	}
	else {
		s = s.substr(i);
	}
	return s;
}

std::string &rtrim(std::string &s)
{
	int i = (int)s.length() - 1;
	while (i >= 0 && isspace(s[i])) {
		i--;
	}
	if (i < 0) {
		s = "";
	}
	else {
		s = s.substr(0, i+1);
	}
	return s;
}

std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

#ifdef _WIN32
List_Directory::List_Directory(std::string filespec) :
	got_first(false),
	done(false)
{
	handle = FindFirstFile(filespec.c_str(), &ffd);
	if (handle == INVALID_HANDLE_VALUE) {
		done = true;
	}
}

List_Directory::~List_Directory()
{
	FindClose(handle);
}

std::string List_Directory::next()
{
	if (done) {
		return "";
	}

	if (got_first == true) {
		if (FindNextFile(handle, &ffd) == 0) {
			done = true;
			return "";
		}
	}
	else {
		got_first = true;
	}

	return ffd.cFileName;
}
#elif !defined ANDROID
List_Directory::List_Directory(std::string filespec) :
	i(0)
{
	gl.gl_pathv = 0;

	int ret = glob(filespec.c_str(), 0, 0, &gl);

	if (ret != 0) {
		i = 0;
	}
}

List_Directory::~List_Directory()
{
	globfree(&gl);
}

std::string List_Directory::next()
{
	if (i >= (int)gl.gl_pathc) {
		i = -1;
	}

	if (i < 0) {
		return "";
	}

	return gl.gl_pathv[i++];
}
#else
List_Directory::List_Directory(std::string filespec)
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(filespec.c_str());

	jmethodID method_id = env->GetMethodID(clazz, "list_dir_start", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
}


List_Directory::~List_Directory()
{
}

std::string List_Directory::next()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "list_dir_next", "()Ljava/lang/String;");

	jstring s = (jstring)env->CallObjectMethod(activity, method_id);

	const char *native = env->GetStringUTFChars(s, 0);

	std::string filename = native;

	env->ReleaseStringUTFChars(s, native);

	env->DeleteLocalRef(s);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return filename;
}
#endif // _WIN32

std::string uppercase(std::string s)
{
	std::string u;
	for (size_t i = 0; i < s.length(); i++) {
		char str[2];
		str[0] = toupper(s[i]);
		str[1] = 0;
		u += str;
	}
	return u;
}

std::string lowercase(std::string s)
{
	std::string l;
	for (size_t i = 0; i < s.length(); i++) {
		char str[2];
		str[0] = tolower(s[i]);
		str[1] = 0;
		l += str;
	}
	return l;
}

std::string unescape_string(std::string s)
{
	std::string ret;
	int p = 0;
	char buf[2];
	buf[1] = 0;

	if (s.length() == 0) {
		return "";
	}

	while (p < (int)s.length()) {
		if (s[p] == '\\') {
			if (p+1 < (int)s.length()) {
				if (s[p+1] == '\\' || s[p+1] == '"') {
					p++;
					buf[0] = s[p];
					ret += buf;
					p++;
				}
				else if (s[p+1] == 'n') {
					p++;
					buf[0] = '\n';
					ret += buf;
					p++;
				}
				else if (s[p+1] == 't') {
					p++;
					buf[0] = '\t';
					ret += buf;
					p++;
				}
				else {
					buf[0] = '\\';
					ret += buf;
					p++;
				}
			}
			else {
				buf[0] = '\\';
				ret += buf;
				p++;
			}
		}
		else {
			buf[0] = s[p];
			ret += buf;
			p++;
		}
	}

	return ret;
}

std::string load_text_from_filesystem(std::string filename)
{
	std::string text;
	std::string line;
	std::ifstream f(filename);

	while (std::getline(f, line)) {
		text += line;
	}

	return text;
}

} // End namespace util

} // End namespace noo
