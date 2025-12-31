#include "shim5/gfx.h"
#include "shim5/image.h"
#include "shim5/shader.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "libutil/libutil.h"

#include "shim5/internal/gfx.h"

using namespace noo;

namespace noo {

namespace gfx {

std::vector<Shader *> Shader::loaded_shaders;
std::vector< std::pair<std::string, Image *> > Shader::bound_images;
Shader *Shader::last_shader;
GLuint Shader::opengl_texture0;

void Shader::static_start()
{
	loaded_shaders.clear();
	bound_images.clear();
	last_shader = 0;
	opengl_texture0 = 0;
}

void Shader::release_all(bool force)
{
	for (size_t i = 0; i < loaded_shaders.size(); i++) {
		loaded_shaders[i]->release(force);
	}

	shim::current_shader = 0;
}

void Shader::reload_all(bool force)
{
	for (size_t i = 0; i < loaded_shaders.size(); i++) {
		loaded_shaders[i]->reload(force);
	}
}

std::vector< std::pair<std::string, Image *> > &Shader::get_bound_images()
{
	return bound_images;
}

void Shader::rebind_opengl_texture0()
{
	if (opengl_texture0 != 0) {
		glActiveTexture_ptr(GL_TEXTURE0);
		PRINT_GL_ERROR("glActiveTexture\n");

		glBindTexture_ptr(GL_TEXTURE_2D, opengl_texture0);
		PRINT_GL_ERROR("glBindTexture\n");
	}
}

void Shader::audit()
{
}

Shader::Shader(OpenGL_Shader *vertex_shader, OpenGL_Shader *fragment_shader, bool is_master_vertex, bool is_master_fragment) :
	opengl_shader(0),
	pos_attrib(-1),
	normal_attrib(-1),
	texcoord_attrib(-1),
	colour_attrib(-1),
	pos_ptr(0),
	normal_ptr(0),
	texcoord_ptr(0),
	colour_ptr(0)
{
	opengl = true;

	this->is_master_vertex = is_master_vertex;
	this->is_master_fragment = is_master_fragment;

	opengl_vertex = vertex_shader;
	opengl_fragment = fragment_shader;

	loaded_shaders.push_back(this);

	reload(true);
}

Shader::~Shader()
{
	for (size_t i = 0; i < loaded_shaders.size(); i++) {
		if (loaded_shaders[i] == this) {
			loaded_shaders.erase(loaded_shaders.begin()+i);
			break;
		}
	}
	release(true);
	if (is_master_vertex) {
		delete opengl_vertex;
	}
	if (is_master_fragment) {
		delete opengl_fragment;
	}
}

void Shader::use()
{
	if (last_shader != 0 && last_shader != this) {
		unbind(last_shader);
	}
	else if (last_shader == this) {
		return;
	}

	glUseProgram_ptr(opengl_shader);
	PRINT_GL_ERROR("glUseProgram\n");

	last_shader = this;
}

void Shader::set_texture(std::string name, Image *image, int unit)
{
	Image *root = image == 0 ? 0 : image->get_root();

	std::pair<std::string, Image *> p;

	for (size_t i = bound_images.size(); (int)i < unit+1; i++) {
		p.first = "";
		p.second = 0;
		bound_images.push_back(p);
	}
	p.first = name;
	p.second = root;
	bound_images[unit] = p;

	GLuint texture = root == 0 ? 0 : root->internal->texture;

	glActiveTexture_ptr(GL_TEXTURE0 + unit);
	PRINT_GL_ERROR("glActiveTexture\n");

	glBindTexture_ptr(GL_TEXTURE_2D, texture);
	PRINT_GL_ERROR("glBindTexture\n");

	if (unit == 0) {
		opengl_texture0 = texture;
	}

	GLint loc = get_uniform_location(name);
	if (loc != -1) {
		glUniform1i_ptr(loc, unit);
	}
}

void Shader::set_matrix(std::string name, glm::mat4 &matrix)
{
	GLint loc = get_uniform_location(name);
	if (loc != -1) {
		glUniformMatrix4fv_ptr(loc, 1, GL_FALSE, glm::value_ptr(matrix));
		PRINT_GL_ERROR("glUniformMatrix4fv\n");
	}
}

void Shader::set_matrix_array(std::string name, int num_matrices, glm::mat4 matrix[])
{
	float *v = new float[16*num_matrices];

	for (int i = 0; i < num_matrices; i++) {
		glm::mat4 m = matrix[i];
		memcpy(v+16*i, glm::value_ptr(m), 16*sizeof(float));
	}

	GLint loc = get_uniform_location(name);
	if (loc != -1) {
		glUniformMatrix4fv_ptr(loc, num_matrices, GL_FALSE, v);
		PRINT_GL_ERROR("glUniformMatrix4fv\n");
	}

	delete[] v;
}

void Shader::set_float(std::string name, float value)
{
	GLint loc = get_uniform_location(name);
	if (loc != -1) {
		glUniform1f_ptr(loc, value);
		PRINT_GL_ERROR("glUniform1f\n");
	}
}

// Taken from Allegro
bool Shader::set_float_vector(std::string name, int num_components, const float *vector, int num_elements)
{
	GLint loc = get_uniform_location(name);

	if (loc < 0) {
		return false;
	}

	switch (num_components) {
		case 1:
			glUniform1fv_ptr(loc, num_elements, vector);
			break;
		case 2:
			glUniform2fv_ptr(loc, num_elements, vector);
			break;
		case 3:
			glUniform3fv_ptr(loc, num_elements, vector);
			break;
		case 4:
			glUniform4fv_ptr(loc, num_elements, vector);
			break;
		default:
			return false;
	}
	PRINT_GL_ERROR("glUniform?fv\n");

	return true;
}

void Shader::set_bool(std::string name, bool value)
{
	GLint loc = get_uniform_location(name);
	if (loc != -1) {
		glUniform1i_ptr(loc, value);
		PRINT_GL_ERROR("glUniform1i\n");
	}
}

void Shader::set_int(std::string name, int value)
{
	GLint loc = get_uniform_location(name);
	if (loc != -1) {
		glUniform1i_ptr(loc, value);
		PRINT_GL_ERROR("glUniform1i\n");
	}
}

bool Shader::set_colour(std::string name, SDL_Color colour)
{
	float vector[4];
	vector[0] = colour.r / 255.0f;
	vector[1] = colour.g / 255.0f;
	vector[2] = colour.b / 255.0f;
	vector[3] = colour.a / 255.0f;

	GLint loc = get_uniform_location(name);

	if (loc < 0) {
		return false;
	}

	glUniform4fv_ptr(loc, 1, vector);

	return true;
}

GLuint Shader::get_opengl_shader()
{
	return opengl_shader;
}

void Shader::release(bool force)
{
	if (this == last_shader) {
		unbind(this);
	}

	if (is_master_vertex && opengl_vertex->shader != 0) {
		glDeleteShader_ptr(opengl_vertex->shader);
		PRINT_GL_ERROR("glDeleteShader\n");
		opengl_vertex->shader = 0;
	}
	if (is_master_fragment && opengl_fragment->shader != 0) {
		glDeleteShader_ptr(opengl_fragment->shader);
		PRINT_GL_ERROR("glDeleteShader\n");
		opengl_fragment->shader = 0;
	}

	if (opengl_shader != 0) {
		glDeleteProgram_ptr(opengl_shader);
		PRINT_GL_ERROR("glDeleteProgram\n");
		opengl_shader = 0;
	}

	uniform_locations.clear();
	
	pos_attrib = normal_attrib = texcoord_attrib = colour_attrib = -1;
}

void Shader::reload(bool force)
{
	if (is_master_vertex && opengl_vertex->shader == 0) {
		opengl_vertex->shader = compile_opengl_vertex_shader(opengl_vertex->source);
	}
	if (is_master_fragment && opengl_fragment->shader == 0) {
		opengl_fragment->shader = compile_opengl_fragment_shader(opengl_fragment->source);
	}

	if (opengl_shader == 0) {
		opengl_shader = glCreateProgram_ptr();
		glAttachShader_ptr(opengl_shader, opengl_vertex->shader);
		PRINT_GL_ERROR("glAttachShader\n");
		glAttachShader_ptr(opengl_shader, opengl_fragment->shader);
		PRINT_GL_ERROR("glAttachShader\n");
		glLinkProgram_ptr(opengl_shader);
		PRINT_GL_ERROR("glLinkProgram\n");
	}
}

void Shader::unbind(Shader *s)
{
	for (size_t i = 0; i < bound_images.size(); i++) {
		std::pair<std::string, Image *> p = bound_images[i];
		s->set_texture(p.first, 0, (int)i);
	}

	bound_images.clear();
	
	if (s == last_shader) {
		last_shader = 0;
	}

	pos_ptr = normal_ptr = texcoord_ptr = colour_ptr = 0;
}

void Shader::set_opengl_attributes(float *pos, float *normal, float *texcoord, float *colour)
{
	if (pos_attrib == -1) {
		pos_attrib = glGetAttribLocation_ptr(opengl_shader, "in_position");
		if (pos_attrib != -1) {
			glEnableVertexAttribArray_ptr(pos_attrib);
			PRINT_GL_ERROR("glEnableVertexAttribArray _ptr(in_position)\n");
		}
	}

	if (normal_attrib == -1) {
		normal_attrib = glGetAttribLocation_ptr(opengl_shader, "in_normal");
		if (normal_attrib != -1) {
			glEnableVertexAttribArray_ptr(normal_attrib);
			PRINT_GL_ERROR("glEnableVertexAttribArray _ptr(in_normal)\n");
		}
	}

	if (texcoord_attrib == -1) {
		texcoord_attrib = glGetAttribLocation_ptr(opengl_shader, "in_texcoord");
		if (texcoord_attrib != -1) {
			glEnableVertexAttribArray_ptr(texcoord_attrib);
			PRINT_GL_ERROR("glEnableVertexAttribArray _ptr(in_texcoord)\n");
		}
	}

	if (colour_attrib == -1) {
		colour_attrib = glGetAttribLocation_ptr(opengl_shader, "in_colour");
		if (colour_attrib != -1) {
			glEnableVertexAttribArray_ptr(colour_attrib);
			PRINT_GL_ERROR("glEnableVertexAttribArray _ptr(in_colour)\n");
		}
	}

	GLuint vbo;
	glGetIntegerv_ptr(GL_ARRAY_BUFFER_BINDING, (GLint *)&vbo);
	PRINT_GL_ERROR("glGetIntegerv\n");

	if (vbo == 0) {
		if (pos_attrib != -1 && pos_ptr != pos) {
			pos_ptr = pos;
			glVertexAttribPointer_ptr(pos_attrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), pos);
			PRINT_GL_ERROR("glVertexAttribPointer _ptr(in_position)\n");
		}
		if (normal_attrib != -1 && normal_ptr != normal) {
			normal_ptr = normal;
			glVertexAttribPointer_ptr(normal_attrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), normal);
			PRINT_GL_ERROR("glVertexAttribPointer _ptr(in_normal)\n");
		}
		if (texcoord_attrib != -1 && texcoord_ptr != texcoord) {
			texcoord_ptr = texcoord;
			glVertexAttribPointer_ptr(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), texcoord);
			PRINT_GL_ERROR("glVertexAttribPointer _ptr(in_texcoord)\n");
		}
		if (colour_attrib != -1 && colour_ptr != colour) {
			colour_ptr = colour;
			glVertexAttribPointer_ptr(colour_attrib, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), colour);
			PRINT_GL_ERROR("glVertexAttribPointer _ptr(in_colour)\n");
		}
	}
	else {
		glVertexAttribPointer_ptr(pos_attrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (GLvoid *)(0*sizeof(float)));
		glVertexAttribPointer_ptr(normal_attrib, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (GLvoid *)(3*sizeof(float)));
		glVertexAttribPointer_ptr(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (GLvoid *)(6*sizeof(float)));
		glVertexAttribPointer_ptr(colour_attrib, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (GLvoid *)(8*sizeof(float)));
	}
}

GLint Shader::get_uniform_location(std::string name)
{
	std::map<std::string, GLint>::iterator it = uniform_locations.find(name);
	if (it == uniform_locations.end()) {
		GLint loc = glGetUniformLocation_ptr(opengl_shader, name.c_str());
		PRINT_GL_ERROR("glGetUniformLocation\n");
		uniform_locations[name] = loc;
		return loc;
	}
	else {
		return (*it).second;
	}
}

GLuint Shader::compile_opengl_shader(GLenum type, std::string source)
{
	GLint status;

	const GLchar *p = (const GLchar *)source.c_str();

	GLuint shader;

	shader = glCreateShader_ptr(type);
	PRINT_GL_ERROR("glCreateShader\n");
	glShaderSource_ptr(shader, 1, &p, 0);
	PRINT_GL_ERROR("glShaderSource\n");
	glCompileShader_ptr(shader);
	PRINT_GL_ERROR("glCompileShader\n");
	glGetShaderiv_ptr(shader, GL_COMPILE_STATUS, &status);
	PRINT_GL_ERROR("glGetShaderiv\n");
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog_ptr(shader, 512, 0, buffer);
		throw util::Error(util::string_printf("%s shader error: %s", buffer, type == GL_VERTEX_SHADER ? "Vertex" : "Fragment"));
	}

	return shader;
}

GLuint Shader::compile_opengl_vertex_shader(std::string source)
{
	return compile_opengl_shader(GL_VERTEX_SHADER, source);
}

GLuint Shader::compile_opengl_fragment_shader(std::string source)
{
	return compile_opengl_shader(GL_FRAGMENT_SHADER, source);
}


std::string Shader::add_opengl_header(bool is_vertex, Precision precision, std::string source)
{
#if defined ANDROID || defined IOS || defined RASPBERRYPI || defined __EMSCRIPTEN__
	std::string p;
	if (is_vertex) {
		if (precision == LOW) {
			p = "lowp";
		}
		else if (precision == MEDIUM) {
			p = "mediump";
		}
		else {
			p = "highp"; // default
		}
	}
	else if (is_vertex == false) {
		if (precision == MEDIUM) {
			p = "mediump";
		}
		else if (precision == HIGH) {
			p = "highp";
		}
		else {
			p = "lowp"; // default
		}
	}
	source = "precision " + p + " float;\n" + source;
#else
	source = std::string("#version 120\n") + source;
#endif
	return source;
}

Shader::OpenGL_Shader *Shader::load_opengl_vertex_shader(std::string source, Precision precision)
{
	OpenGL_Shader *shader = new OpenGL_Shader;
	shader->type = GL_VERTEX_SHADER;
	shader->precision = precision;
	shader->source = source = add_opengl_header(true, precision, source);
	shader->shader = 0;
	return shader;
}

Shader::OpenGL_Shader *Shader::load_opengl_fragment_shader(std::string source, Precision precision)
{
	OpenGL_Shader *shader = new OpenGL_Shader;
	shader->type = GL_FRAGMENT_SHADER;
	shader->precision = precision;
	shader->source = source = add_opengl_header(true, precision, source);
	shader->shader = 0;
	return shader;
}

} // End namespace gfx

} // End namespace noo
