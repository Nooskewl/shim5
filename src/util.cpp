#include "shim5/cpa.h"
#include "shim5/shim.h"
#include "shim5/vertex_cache.h"
#include "shim5/util.h"
#include "shim5/langid.h"
#include "shim5/internal/util.h"

#include <sys/stat.h>
#include <sys/time.h>

#include <shlobj.h>
#include <dbghelp.h>

#include <fstream>

#include "shim5/internal/gfx.h"
#include "shim5/internal/util.h"

static bool appdata_dir_set = false;
static std::string appdata_dir;

using namespace noo;

static FILE *log_file;

namespace noo {

namespace util {

namespace internal {

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

static void print_string_console(const char *string)
{
	OutputDebugString(string);
	printf("%s", string);
}

void print_string(int level, const char *string)
{
	if (level <= 3 && log_file) { // Don't do verbose logging to log file
		fprintf(log_file, "%s", string);
	}
	if (level <= shim::error_level) {
		print_string_console(string);
	}
}

std::string get_game_name()
{
	if (shim::game_name != "") {
		return shim::game_name;
	}
	return "Nooskewl Shim";
}

} // End namespace internal

bool basic_start()
{
	log_file = 0;

	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+error-level")) > 0) {
		shim::error_level = atoi(shim::argv[index+1]);
	}

	return true;
}

bool static_start()
{
	if (shim::logging) {
		std::string log_filename = get_appdata_dir() + "/log.txt";
		log_file = fopen(log_filename.c_str(), "w");
	}

	return true;
}

void static_end()
{
	internal::close_log_file();
}

bool start()
{
	srand((uint32_t)time(0));

	return true;
}

void end()
{
}

void errormsg(const char *fmt, ...)
{
	std::string prefix = internal::get_game_name();
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
	internal::print_string(1, buf);
	va_end(v);
	delete[] fmt2;
}

void errormsg(std::string s)
{
	errormsg(s.c_str());
}

void infomsg(const char *fmt, ...)
{
	std::string prefix = internal::get_game_name();
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
	internal::print_string(2, buf);
	va_end(v);
	delete[] fmt2;
}

void infomsg(std::string s)
{
	infomsg(s.c_str());
}

void debugmsg(const char *fmt, ...)
{
	std::string prefix = internal::get_game_name();
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
	internal::print_string(3, buf);
	va_end(v);
	delete[] fmt2;
}

void debugmsg(std::string s)
{
	debugmsg(s.c_str());
}

void verbosemsg(const char *fmt, ...)
{
	std::string prefix = internal::get_game_name();
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
	internal::print_string(4, buf);
	va_end(v);
	delete[] fmt2;
}

void verbosemsg(std::string s)
{
	verbosemsg(s.c_str());
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
					size_t read;
					size_t to_read = MIN(*sz-count, chunk_size);
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

	if (SDL_ReadIO(file, buf, size) != (size_t)size) {
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

	if (SDL_ReadIO(file, buf, _sz) != (size_t)_sz) {
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

	if (SDL_ReadIO(file, buf, _sz) != (size_t)_sz) {
		throw LoadError(filename);
	}

	SDL_CloseIO(file);

	if (sz) {
		*sz = _sz;
	}

	return buf;
}

Uint64 file_date(std::string filename)
{
	struct stat s;
	if (stat(filename.c_str(), &s) == 0) {
		return s.st_ctime;
	}
	else {
		return -1;
	}
}

time_t utc_secs()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec;
}

void mkdir(std::string path)
{
	_mkdir(path.c_str());
}

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

std::string get_system_language()
{
	return get_system_language_windows();
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

std::string get_standard_path(Path_Type type, bool create)
{
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
	appdata += "/" + internal::get_game_name();
	mkdir(appdata);
	return appdata;
}

std::string get_savegames_dir()
{
	std::string path;

	path = util::get_standard_path(util::SAVED_GAMES, true);
	path += "/" + shim::game_name;
	util::mkdir(path);

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

int check_args(int argc, char **argv, std::string arg)
{
	if (argc <= 0 || argv == 0) {
		return -1;
	}

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], arg.c_str())) {
			return i;
		}
	}
	return -1;
}

bool bool_arg(bool default_value, int argc, char **argv, std::string arg)
{
	if (argc <= 0 || argv == 0) {
		return default_value;
	}

	std::string on = std::string("+") + arg;
	std::string off = std::string("-") + arg;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], on.c_str())) {
			default_value = true;
		}
		else if (!strcmp(argv[i], off.c_str())) {
			default_value = false;
		}
	}

	return default_value;
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
		std::string prefix = internal::get_game_name();
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
		internal::print_string(3, buf);
		va_end(v);
		delete[] fmt2;
	}
}

void open_with_system(std::string filename)
{
	if (gfx::internal::gfx_context.fullscreen) {
		ShowWindow(gfx::internal::gfx_context.hwnd, SW_MINIMIZE);
	}
	ShellExecute(0, 0, filename.c_str(), 0, 0 , SW_SHOW);
}

void open_url(std::string url)
{
	open_with_system(url);
}

} // end namespace util

} // end namespace noo
