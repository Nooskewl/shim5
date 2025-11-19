#ifndef NOO_SHADER_H
#define NOO_SHADER_H

#include "shim5/main.h"

namespace noo {

namespace gfx {

class Image;

class Shader
{
public:
	enum Precision {
		UNSPECIFIED = 0,
		LOW,
		MEDIUM,
		HIGH
	};

	static void static_start();
	static void release_all(bool force);
	static void reload_all(bool force);
	static std::vector< std::pair<std::string, Image *> > &get_bound_images();
	static void rebind_opengl_texture0();
	static void audit();

	struct OpenGL_Shader {
		GLenum type;
		Precision precision;
		GLuint shader;
		std::string source;
	};
	static OpenGL_Shader *load_opengl_vertex_shader(std::string source, Precision precision = LOW);
	static OpenGL_Shader *load_opengl_fragment_shader(std::string source, Precision precision = LOW);
	
	SHIM5_EXPORT Shader(OpenGL_Shader *vertex_shader, OpenGL_Shader *fragment_shader, bool is_master_vertex = true, bool is_master_fragment = true);

	SHIM5_EXPORT ~Shader();

	SHIM5_EXPORT void use();

	SHIM5_EXPORT void set_texture(std::string name, Image *image, int unit = 0);
	SHIM5_EXPORT void set_matrix(std::string name, glm::mat4 &matrix);
	SHIM5_EXPORT void set_matrix_array(std::string name, int num_matrices, glm::mat4 matrix[]);
	SHIM5_EXPORT void set_float(std::string name, float value);
	SHIM5_EXPORT bool set_float_vector(std::string name, int num_components, const float *vector, int num_elements);
	SHIM5_EXPORT void set_bool(std::string name, bool value);
	SHIM5_EXPORT void set_int(std::string name, int value);
	SHIM5_EXPORT bool set_colour(std::string name, SDL_Color colour);

	SHIM5_EXPORT GLuint get_opengl_shader();
	SHIM5_EXPORT void set_opengl_attributes(float *pos, float *normal, float *texcoord, float *colour);

private:
	static std::string add_opengl_header(bool is_vertex, Precision precision, std::string source);
	static GLuint compile_opengl_vertex_shader(std::string source);
	static GLuint compile_opengl_fragment_shader(std::string source);
	static GLuint compile_opengl_shader(GLenum type, std::string source);

	// Formerly Internal --
	SHIM5_EXPORT void release(bool force);
	SHIM5_EXPORT void reload(bool force);
	SHIM5_EXPORT void unbind(Shader *s);
	SHIM5_EXPORT GLint get_uniform_location(std::string name);

	bool opengl;
	bool is_master_vertex;
	bool is_master_fragment;

	// OpenGL
	OpenGL_Shader *opengl_vertex;
	OpenGL_Shader *opengl_fragment;
	GLuint opengl_shader;
	GLint pos_attrib, normal_attrib, texcoord_attrib, colour_attrib;
	float *pos_ptr, *normal_ptr, *texcoord_ptr, *colour_ptr;
	std::map<std::string, GLint> uniform_locations;

	static std::vector<Shader *> loaded_shaders;
	static std::vector< std::pair<std::string, Image *> > bound_images;
	static Shader *last_shader;
	static GLuint opengl_texture0;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_SHADER_H

