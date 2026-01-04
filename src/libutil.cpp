#include <string>
#include <fstream>

#include "libutil/libutil.h"

#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <shlobj.h>
#include <dbghelp.h>
#include "shim5/langid.h"
#else
#include <sys/types.h>
#endif

#ifdef ANDROID
#include <jni.h>
#endif

#ifdef STEAMWORKS
#include "shim5/steamworks.h"
#endif

static bool appdata_dir_set = false;
static std::string appdata_dir;

namespace noo {

namespace shim {

std::string organisation_name;
std::string game_name;

} // End namespace shim

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
	LANGID l = GetUserDefaultUILanguage();

	// returns names in Steam format

	if (
		l == Arabic ||
		l == Arabic_Algeria ||
		l == Arabic_Bahrain ||
		l == Arabic_Egypt ||
		l == Arabic_Iraq ||
		l == Arabic_Jordan ||
		l == Arabic_Kuwait ||
		l == Arabic_Lebanon ||
		l == Arabic_Libya ||
		l == Arabic_Morocco ||
		l == Arabic_Oman ||
		l == Arabic_Qatar ||
		l == Arabic_Saudi_Arabia ||
		l == Arabic_Syria ||
		l == Arabic_Tunisia ||
		l == Arabic_U_A_E_ ||
		l == Arabic_Yemen) {
		return "arabic";
	}

	if (
		l == Bulgarian ||
		l == Bulgarian_Bulgaria) {
		return "bulgarian";
	}

	if (	
		l == Chinese_Simplified ||
		l == Chinese_Simplified_Legacy ||
		l == Chinese_Simplified__PRC ||
		l == Chinese_Simplified__Singapore) {
		return "schinese";
	}

	if (
		l == Chinese_Traditional ||
		l == Chinese_Traditional_Legacy ||
		l == Chinese_Traditional__Hong_Kong_S_A_R_ ||
		l == Chinese_Traditional__Macao_S_A_R_ ||
		l == Chinese_Traditional__Taiwan) {
		return "tchinese";
	}

	if (
		l == Czech ||
		l == Czech_Czech_Republic) {
		return "czech";
	}

	if (
		l == Danish ||
		l == Danish_Denmark) {
		return "danish";
	}

	if (
		l == Dutch ||
		l == Dutch_Belgium ||
		l == Dutch_Netherlands) {
		return "dutch";
	}

	if (
		l == Finnish ||
		l == Finnish_Finland) {
		return "finnish";
	}

	if (
		l == French ||
		l == French_Belgium ||
		l == French_Cameroon ||
		l == French_Canada ||
		l == French_Caribbean ||
		l == French_Congo_DRC ||
		l == French_Cote_dIvoire ||
		l == French_France ||
		l == French_Haiti ||
		l == French_Luxembourg ||
		l == French_Mali ||
		l == French_Monaco ||
		l == French_Morocco ||
		l == French_Reunion ||
		l == French_Senegal ||
		l == French_Switzerland) {
		return "french";
	}

	if (
		l == German ||
		l == German_Austria ||
		l == German_Germany ||
		l == German_Liechtenstein ||
		l == German_Luxembourg ||
		l == German_Switzerland) {
		return "german";
	}

	if (
		l == Greek ||
		l == Greek_Greece) {
		return "greek";
	}

	if (
		l == Hungarian ||
		l == Hungarian_Hungary) {
		return "hungarian";
	}

	if (
		l == Indonesian ||
		l == Indonesian_Indonesia) {
		return "indonesian";
	}

	if (
		l == Italian ||
		l == Italian_Italy ||
		l == Italian_Switzerland) {
		return "italian";
	}

	if (
		l == Japanese ||
		l == Japanese_Japan) {
		return "japanese";
	}

	if (
		l == Korean ||
		l == Korean_Korea) {
		return "korean";
	}

	if (
		l == Norwegian ||
		l == Norwegian_Bokmal ||
		l == Norwegian_Nynorsk ||
		l == Norwegian__Bokmal_Norway ||
		l == Norwegian__Nynorsk_Norway) {
		return "norwegian";
	}

	if (
		l == Polish ||
		l == Polish_Poland) {
		return "polish";
	}

	if (
		l == Portuguese ||
		l == Portuguese_Portugal) {
		return "portuguese";
	}

	if (
		l == Portuguese_Brazil) {
		return "brazilian";
	}

	if (
		l == Romanian ||
		l == Romanian_Moldova ||
		l == Romanian_Romania) {
		return "romanian";
	}

	if (
		l == Russian ||
		l == Russian_Moldova ||
		l == Russian_Russia) {
		return "russian";
	}

	if (
		l == Spanish ||
		l == Spanish_Spain) {
		return "spanish";
	}

	if (
		l == Spanish_Argentina ||
		l == Spanish_Venezuela ||
		l == Spanish_Bolivia ||
		l == Spanish_Chile ||
		l == Spanish_Colombia ||
		l == Spanish_Costa_Rica ||
		l == Spanish_Cuba ||
		l == Spanish_Dominican_Republic ||
		l == Spanish_Ecuador ||
		l == Spanish_El_Salvador ||
		l == Spanish_Guatemala ||
		l == Spanish_Honduras ||
		l == Spanish_Latin_America ||
		l == Spanish_Mexico ||
		l == Spanish_Nicaragua ||
		l == Spanish_Panama ||
		l == Spanish_Paraguay ||
		l == Spanish_Peru ||
		l == Spanish_Puerto_Rico ||
		l == Spanish_United_States ||
		l == Spanish_Uruguay) {
		return "latam";
	}

	if (
		l == Swedish ||
		l == Swedish_Finland ||
		l == Swedish_Sweden) {
		return "swedish";
	}

	if (
		l == Thai ||
		l == Thai_Thailand) {
		return "thai";
	}

	if (
		l == Turkish ||
		l == Turkish_Turkey) {
		return "turkish";
	}

	if (
		l == Ukrainian ||
		l == Ukrainian_Ukraine) {
		return "ukrainian";
	}

	if (
		l == Vietnamese ||
		l == Vietnamese_Vietnam) {
		return "vietnamese";
	}

	return "english";
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
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
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
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
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
#elif defined __EMSCRIPTEN__
	return "english";
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
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
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
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
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
		text += line + "\n";
	}

	return text;
}

std::string remove_quotes(std::string s)
{
       int start = 0;
       int count = s.length();

       if (s[0] == '"') {
               start++;
               count--;
       }

       if (s[s.length()-1] == '"') {
               count--;
       }

       return s.substr(start, count);
}

std::string itos(int i)
{
	char buf[20];
	snprintf(buf, 20, "%d", i);
	return std::string(buf);
}

static std::string get_game_name()
{
	if (shim::game_name != "") {
		return shim::game_name;
	}
	return "Nooskewl Shim";
}

std::string get_standard_path(Path_Type type, bool create)
{
#ifdef _WIN32
	if (type == SAVED_GAMES) {
		std::string userprofile = getenv("USERPROFILE");
		if (userprofile != "") {
			userprofile += "\\Saved Games";
			if (create) {
				mkdir(userprofile);
			}
			return userprofile;
		}
	}

	int i;
	if (type == DOCUMENTS) {
		i = CSIDL_PERSONAL;
	}
	else if (type == APPDATA) {
		i = CSIDL_APPDATA;
	}
	else if (type == HOME) {
		i = CSIDL_PROFILE;
	}
	else {
		return "";
	}

	if (create) {
		i |= CSIDL_FLAG_CREATE;
	}

	char buf[MAX_PATH];

	HRESULT result = SHGetFolderPath(
		//gfx::internal::gfx_context.hwnd,
		nullptr,
		i,
		NULL,
		0,
		buf
	);

	if (result == S_OK) {
		return std::string(buf);
	}

	return "";
#elif (defined __linux__ || defined __EMSCRIPTEN__) && !defined ANDROID
	std::string path = getenv("HOME");
	if (create) {
		mkdir(path);
	}
	if (type == DOCUMENTS) {
		path += "/Documents";
	}
	else if (type == APPDATA) {
		path += "/.config";
	}
	if (create) {
		mkdir(path);
	}
	return path;
#elif defined ANDROID
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id;
	
	if (type == SAVED_GAMES) {
		method_id = env->GetMethodID(clazz, "getSDCardDir", "()Ljava/lang/String;");
	}
	else {
		method_id = env->GetMethodID(clazz, "getAppdataDir", "()Ljava/lang/String;");
	}

	jstring s = (jstring)env->CallObjectMethod(activity, method_id);

	const char *native = env->GetStringUTFChars(s, 0);

	std::string path = native;

	if (type == SAVED_GAMES) {
		path += "/" + shim::game_name;
		if (create) {
			mkdir(path.c_str());
		}
	}

	env->ReleaseStringUTFChars(s, native);

	env->DeleteLocalRef(s);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return path;
#elif defined IOS
	std::string path = ios_get_standard_path(type);
	if (create) {
		mkdir(path);
	}
	return path;
#else
	std::string path = macosx_get_standard_path(type);
	if (create) {
		mkdir(path);
	}
	return path;
#endif
}

std::string get_appdata_dir()
{
	if (appdata_dir_set) {
		return appdata_dir;
	}

	std::string appdata = get_standard_path(APPDATA, true);
	if (shim::organisation_name != "") {
		appdata += "/" + shim::organisation_name;
		mkdir(appdata);
	}
	appdata += "/" + get_game_name();
	mkdir(appdata);
	return appdata;
}

std::string get_savegames_dir()
{
	std::string path;

#ifdef ANDROID
	path = util::get_standard_path(util::SAVED_GAMES, true);
#elif defined _WIN32
	path = util::get_standard_path(util::SAVED_GAMES, true);
	path += "/" + shim::game_name;
	util::mkdir(path);
#else
	path = util::get_appdata_dir();
#endif

	return path;
}

void set_appdata_dir(std::string appdata_dir, bool create)
{
	if (create) {
		std::string s;
		for (size_t i = 0; i < appdata_dir.length(); i++) {
			char c = appdata_dir[i];
			if (c == '/' || c == '\\') {
				mkdir(s);
			}
			char cs[2];
			cs[0] = c;
			cs[1] = 0;
			s += cs;
		}
	}
	::appdata_dir = appdata_dir;
	appdata_dir_set = true;
}

} // End namespace util

} // End namespace noo
