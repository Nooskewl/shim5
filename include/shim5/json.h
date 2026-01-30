#ifndef NOO_JSON_H
#define NOO_JSON_H

#include "shim5/main.h"

namespace noo {

namespace util {

class Trigger;

class SHIM5_EXPORT JSON {
public:
	struct SHIM5_EXPORT Node {
		enum Type {
			NONE,
			ARRAY,
			HASH,
			INT,
			BOOL,
			DOUBLE,
			STRING,
			BYTE
		};

		typedef std::map<std::string, Node *> NodeMap;
		typedef NodeMap::iterator NodeIt;

		std::string key;
		std::string value; // can be [array] or [hash]
		std::vector<Node *> children;
		std::map<std::string, Node *> child_map;

		Type type;
		void *userdata;

		Node *parent;

		Trigger *trigger; // run() is called whenever the onscreen vars viewer changes this variable (after it has been changed)

		bool readonly; // devsettings can't modify it

		SHIM5_EXPORT Node *find(std::string loc); // can search nested nodes by separating with >

		SHIM5_EXPORT std::string to_json(int indent = 0); // get the whole tree as a string in JSON format

		SHIM5_EXPORT std::string as_string(); // removes quotes if any
		// these do not trim quotes before conversion, so e.g. "74" will not be correct
		SHIM5_EXPORT int as_int();
		SHIM5_EXPORT bool as_bool();
		SHIM5_EXPORT double as_double();
		SHIM5_EXPORT Uint8 as_byte();

		// If userdata is set, these expect it to be the correct type. these also call trigger->run if trigger != NULL
		SHIM5_EXPORT void set_string(std::string s);
		SHIM5_EXPORT void set_int(int i);
		SHIM5_EXPORT void set_bool(bool b);
		SHIM5_EXPORT void set_double(double f);
		SHIM5_EXPORT void set_byte(Uint8 b);

		SHIM5_EXPORT void set_type(Type t);
		SHIM5_EXPORT void set_userdata(void *u);

		SHIM5_EXPORT Type get_type();
		SHIM5_EXPORT int size();
		SHIM5_EXPORT std::string get_value();

		// sets type, userdata and value
		SHIM5_EXPORT void set_type_string(void *userdata, std::string s);
		SHIM5_EXPORT void set_type_int(void *userdata, int i);
		SHIM5_EXPORT void set_type_bool(void *userdata, bool b);
		SHIM5_EXPORT void set_type_double(void *userdata, double f);
		SHIM5_EXPORT void set_type_byte(void *userdata, Uint8 b);

		// returns value at loc of given type, sets type and userdata. If loc doesn't exist, it just returns the default value passed in and/or adds it if add is true
		SHIM5_EXPORT std::string get_nested_string(std::string loc, void *userdata, std::string def, bool add = true, bool readonly = false);
		SHIM5_EXPORT int get_nested_int(std::string loc, void *userdata, int def, bool add = true, bool readonly = false);
		SHIM5_EXPORT bool get_nested_bool(std::string loc, void *userdata, bool def, bool add = true, bool readonly = false); double get_nested_double(std::string loc, void *userdata, double def, bool add = true, bool readonly = false);
		SHIM5_EXPORT Uint8 get_nested_byte(std::string loc, void *userdata, Uint8 def, bool add = true, bool readonly = false);

		SHIM5_EXPORT void add_nested_string(std::string loc, void *userdata, std::string val, Trigger *trigger = NULL, bool readonly = false);
		SHIM5_EXPORT void add_nested_int(std::string loc, void *userdata, int val, Trigger *trigger = NULL, bool readonly = false);
		SHIM5_EXPORT void add_nested_bool(std::string loc, void *userdata, bool val, Trigger *trigger = NULL, bool readonly = false);
		SHIM5_EXPORT void add_nested_double(std::string loc, void *userdata, double val, Trigger *trigger = NULL, bool readonly = false);
		SHIM5_EXPORT void add_nested_byte(std::string loc, void *userdata, Uint8 val, Trigger *trigger = NULL, bool readonly = false);
		SHIM5_EXPORT void add_nested_array(std::string loc);
		SHIM5_EXPORT void add_nested_hash(std::string loc);

		SHIM5_EXPORT bool remove_child(Node *child, bool del = true); // del = delete also

		SHIM5_EXPORT void update_value(); // read value from userdata, store in value

		SHIM5_EXPORT Node *clone(Node *parent);
		SHIM5_EXPORT void merge(Node *n, Node *parent);
	
	private:
		Node *add_child(Node *child);
		Node *add_child(std::string key, std::string value, Type type = NONE, void *userdata = NULL); // if string, add quotes to value before calling
		void add_nested(std::string loc, Node *add); // doesn't check if node exists, just adds it
	};

	SHIM5_EXPORT JSON(std::string filename, bool load_from_filesystem = false);
	SHIM5_EXPORT JSON(SDL_IOStream *file);
	SHIM5_EXPORT JSON(bool array);
	SHIM5_EXPORT ~JSON();

	SHIM5_EXPORT Node *get_root();
	SHIM5_EXPORT bool remove(std::string loc, bool del = true);

private:
	void read(std::string filename, bool load_from_filesystem = false);
	void read(SDL_IOStream *file);
	int read_char(SDL_IOStream *file);
	void unget(int c);
	void skip_whitespace(SDL_IOStream *file);
	std::string read_token(SDL_IOStream *file, bool is_string);
	std::string read_string(SDL_IOStream *file);
	std::string read_value(SDL_IOStream *file);
	void read_array(Node *node, SDL_IOStream *file);
	void read_hash(Node *node, SDL_IOStream *file);
	void parse_node(Node *node, SDL_IOStream *file);
	void read(Node *node, SDL_IOStream *file);
	void destroy(Node *node);

	Node *root;
	int ungot;
	int line;
};

} // End namespace util

} // End namespace noo

#endif // NOO_JSON_H
