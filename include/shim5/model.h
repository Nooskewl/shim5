#ifndef NOO_MODEL_H
#define NOO_MODEL_H

#include "shim5/main.h"

namespace noo {

namespace gfx {

class Image;

class Model {
public:
	static void static_start();
	static void update_all();

	struct Node;

	EXPORT_STRUCT_ALIGN(Weights, 16) {
		std::string name;
		float *weights;
		glm::mat4 transform;

		Weights();
		~Weights();

		// For 16 byte alignment to make glm::mat4 able to use SIMD
	};

	EXPORT_STRUCT_ALIGN(Bone, 16) {
		std::string name;
		std::vector<glm::mat4> frames;
		glm::mat4 combined_transform;

		// For 16 byte alignment to make glm::mat4 able to use SIMD
	};

	struct Animation {
		std::string name;
		std::map<std::string, Bone *> bones;
		bool is_precalculated;
		int precalc_fps;
		std::vector<GLuint> vbos;
	};

	struct Influence {
		std::vector<Weights *> weights;
		std::vector<Bone *> bones;
	};

	EXPORT_STRUCT_ALIGN(Node, 16) {
		std::string name;
		Node *parent;
		glm::mat4 transform;
		float *vertices;
		float *animated_vertices;
		Uint32 *face_textures;
		std::vector<Image *> textures;
		Uint32 num_triangles;
		Uint32 num_vertices;
		Influence *influences;
		std::vector<Weights *> weights;
		std::vector<Node *> children;

		float min_x, min_y, min_z;
		float max_x, max_y, max_z;

		Node();
		~Node();

		Node *find(std::string name);
		void create_arrays(float *v, int *f, float *n, int *i, float *t, float *c, int num_triangles);

		void animate(Animation *animation, int frame, glm::mat4 *transform);

		// For 16 byte alignment to make glm::mat4 able to use SIMD
	};

	SHIM5_EXPORT Model(std::string filename, bool use_vbo = true, int fps = 30, bool load_from_filesystem = false);
	SHIM5_EXPORT ~Model();

	SHIM5_EXPORT std::vector<Node *> get_nodes();
	SHIM5_EXPORT Node *find(std::string name);
	SHIM5_EXPORT void add_node(Node *node);
	SHIM5_EXPORT Animation *get_animation(std::string name);
	SHIM5_EXPORT void set_animation(std::string name, util::Callback finished_callback = 0, void *finished_callback_data = 0);
	SHIM5_EXPORT std::string get_current_animation();
	SHIM5_EXPORT int get_current_frame();
	SHIM5_EXPORT void start();
	SHIM5_EXPORT void stop();
	SHIM5_EXPORT void reset();

	SHIM5_EXPORT void precalculate_animations(int fps);

	SHIM5_EXPORT void draw();
	SHIM5_EXPORT void draw_tinted(SDL_Color tint);
	SHIM5_EXPORT void draw_textured();
	SHIM5_EXPORT void draw_tinted_textured(SDL_Color tint);

	SHIM5_EXPORT bool save_binary_model(std::string filename);
	
	SHIM5_EXPORT float *calc_frame(std::string anim_name, int frame);
	
	SHIM5_EXPORT void set_animation_finished_callback(util::Callback callback, void *callback_data = 0);

private:
	SHIM5_EXPORT void read(std::string filename, bool load_from_filesystem);
	SHIM5_EXPORT int read_byte(SDL_IOStream *file);
	SHIM5_EXPORT void unget(int c);
	SHIM5_EXPORT void skip_whitespace(SDL_IOStream *file);
	SHIM5_EXPORT void destroy(Node *node);
	SHIM5_EXPORT void destroy(Animation *animation);
	SHIM5_EXPORT std::string read_word(SDL_IOStream *file);
	SHIM5_EXPORT Node *read_text_frame(SDL_IOStream *file);
	SHIM5_EXPORT void skip_section(SDL_IOStream *file);
	SHIM5_EXPORT void read_text_model(SDL_IOStream *file);
	SHIM5_EXPORT Animation *read_animationset(SDL_IOStream *file);
	SHIM5_EXPORT Bone *read_animation(SDL_IOStream *file);
	SHIM5_EXPORT void precalculate_animation(std::string name, int fps);
	SHIM5_EXPORT void draw(SDL_Color tint, bool textured);
	SHIM5_EXPORT void read_binary_model(SDL_IOStream *file);
	SHIM5_EXPORT void write_string(SDL_IOStream *file, std::string s);
	SHIM5_EXPORT void write_matrix(SDL_IOStream *file, glm::mat4 &matrix);
	SHIM5_EXPORT void save_binary_frame(SDL_IOStream *file, Node *n);
	SHIM5_EXPORT void save_binary_animation(SDL_IOStream *file, Animation *a);
	SHIM5_EXPORT std::string read_string(SDL_IOStream *file);
	SHIM5_EXPORT glm::mat4 read_matrix(SDL_IOStream *file);
	SHIM5_EXPORT Node *read_binary_frame(SDL_IOStream *file);
	SHIM5_EXPORT Animation *read_binary_animation(SDL_IOStream *file);

	struct Instance {
		std::map<std::string, Animation *> animations;
		std::string current_animation;
		util::Callback finished_callback;
		void *finished_callback_data;
		bool started;
		Uint32 elapsed;
		Uint32 frames_per_second;
		Model *model;
		std::string filename;
		bool is_clone;
	};

	static std::vector< std::pair<std::string, Instance *> > loaded_models;
	static int model_count;

	std::vector<Node *> roots;
	std::vector<int> ungot;
	int line;

	std::string filename;

	Instance *instance;

	std::map<std::string, float **> precalculated;

	bool use_vbo;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_MODEL_H
