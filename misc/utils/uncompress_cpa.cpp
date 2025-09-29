#include "shim5/shim5.h"

using namespace noo;

int main(int argc, char **argv)
{
	try {
		shim::static_start_all();

		shim::argc = argc+1;
		shim::argv = new char *[shim::argc];
		for (int i = 0; i < shim::argc-1; i++) {
			shim::argv[i] = new char[strlen(argv[i])+1];
			strcpy(shim::argv[i], argv[i]);
		}
		shim::argv[shim::argc-1] = new char[strlen("-fullscreen")+1];
		strcpy(shim::argv[shim::argc-1], "-fullscreen");
		shim::hide_window = true;
		shim::use_cwd = true;
		shim::error_level = 3;
		shim::log_tags = false;

		shim::start_all(100, 100, false, 100, 100);

		util::CPA *cpa = new util::CPA();

		std::vector<std::string> fn = cpa->get_all_filenames();

		for (size_t i = 0; i < fn.size(); i++) {
			std::string s = fn[i];
			int p = s.length();
			while (p > 0 && s[p] != '/') {
				p--;
			}
			std::string dir = "data/" + s.substr(0, p);
			for (size_t j = 0; j < dir.length(); j++) {
				if (dir[j] == '\\') {
					dir[j] = '/';
				}
			}
			if (dir[dir.length()-1] == '/') {
				dir = dir.substr(dir.length()-1);
			}
			util::Tokenizer t(dir, '/');
			std::string tok;
			std::string full = "";
			while ((tok = t.next()) != "") {
				full = full + (full == "" ? "" : "/") + tok;
				util::mkdir(full);
			}
			FILE *out = fopen(("data/" + s).c_str(), "wb");
			int sz;
			SDL_IOStream *in = cpa->open(s, &sz);
			for (int j = 0; j < sz; j++) {
				int c = util::SDL_fgetc(in);
				fputc(c, out);
			}
			cpa->close(in);
			fclose(out);

		}

		shim::end_all();

		shim::static_end();
	}
	catch (util::Error &e) {
		util::errormsg("Fatal error: %s\n", e.error_message.c_str());
	}

	return 0;
}
