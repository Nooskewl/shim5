#ifndef NOO_MAIN_H
#define NOO_MAIN_H

#ifdef SHIM5_STATIC
#define SHIM5_EXPORT
#else
#ifdef SHIM5_BUILD
#define SHIM5_EXPORT __declspec(dllexport)
#else
#define SHIM5_EXPORT __declspec(dllimport)
#endif
#endif

#ifdef __GNUC__
#define EXPORT_STRUCT_ALIGN(x, n) struct SHIM5_EXPORT __attribute__((aligned(n))) x
#define EXPORT_CLASS_ALIGN(x, n) class SHIM5_EXPORT __attribute__((aligned(n))) x
#define ALIGN(n) class __attribute__((aligned(n)))
#else
#define EXPORT_STRUCT_ALIGN(x, n) struct SHIM5_EXPORT __declspec(align(n)) x
#define EXPORT_CLASS_ALIGN(x, n) class SHIM5_EXPORT __declspec(align(n)) x
#define ALIGN(n) class __declspec(align(n))
#endif

#include <cctype>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <sys/types.h>

#ifdef __GNUC__
#include <utime.h>
#endif

#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>

typedef char GLchar;
typedef void (APIENTRY * glStencilFuncSeparate_func)(GLenum face, GLenum func, GLint ref, GLuint mask);
typedef void (APIENTRY * glStencilOpSeparate_func)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRY * glBindFramebuffer_func)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY * glDeleteRenderbuffers_func)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY * glGenFramebuffers_func)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY * glGenRenderbuffers_func)(GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY * glBindRenderbuffer_func)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY * glFramebufferTexture2D_func)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * glRenderbufferStorage_func)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef GLenum (APIENTRY * glCheckFramebufferStatus_func)(GLenum target);
typedef void (APIENTRY * glDeleteFramebuffers_func)(GLsizei n, const GLuint * framebuffers);
typedef void (APIENTRY * glFramebufferRenderbuffer_func)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY * glUseProgram_func)(GLuint program);
typedef void (APIENTRY * glUniform1f_func)(GLint location, GLfloat v0);
typedef void (APIENTRY * glUniform2f_func)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * glUniform3f_func)(GLint location, GLfloat v0, GLfloat v1, GLfloat v3);
typedef void (APIENTRY * glUniform4f_func)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY * glUniform1i_func)(GLint location, GLint v0);
typedef void (APIENTRY * glUniform2i_func)(GLint location, GLint v0, GLint v1);
typedef void (APIENTRY * glUniform3i_func)(GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY * glUniform4i_func)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRY * glUniform1fv_func)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform2fv_func)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform3fv_func)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform4fv_func)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform1iv_func)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniform2iv_func)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniform3iv_func)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniform4iv_func)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniformMatrix2fv_func)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * glUniformMatrix3fv_func)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * glUniformMatrix4fv_func)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * glDeleteShader_func)(GLuint shader);
typedef GLuint (APIENTRY * glCreateProgram_func)(void);
typedef void (APIENTRY * glDeleteProgram_func)(GLuint program);
typedef void (APIENTRY * glAttachShader_func)(GLuint program, GLuint shader);
typedef void (APIENTRY * glLinkProgram_func)(GLuint program);
typedef GLint (APIENTRY * glGetAttribLocation_func)(GLuint program, const GLchar *name);
typedef void (APIENTRY * glGetTexImage_func)(GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
typedef void (APIENTRY * glEnableVertexAttribArray_func)(GLuint index);
typedef void (APIENTRY * glVertexAttribPointer_func)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
typedef GLint (APIENTRY * glGetUniformLocation_func)(GLuint program, const GLchar *name);
typedef void (APIENTRY * glShaderSource_func)(GLuint shader, GLsizei count, const GLchar * const *string, const GLint *length);
typedef void (APIENTRY * glCompileShader_func)(GLuint shader);
typedef void (APIENTRY * glGetShaderiv_func)(GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY * glGetShaderInfoLog_func)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef GLuint (APIENTRY * glCreateShader_func)(GLenum shaderType);
typedef void (APIENTRY * glBlendFunc_func)(GLenum, GLenum);
typedef void (APIENTRY * glBlendFuncSeparate_func)(GLenum, GLenum, GLenum, GLenum);
typedef void (APIENTRY * glEnable_func)(GLenum);
typedef void (APIENTRY * glDisable_func)(GLenum);
typedef void (APIENTRY * glFrontFace_func)(GLenum);
typedef void (APIENTRY * glCullFace_func)(GLenum);
typedef void (APIENTRY * glScissor_func)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY * glViewport_func)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY * glClearColor_func)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (APIENTRY * glClear_func)(GLbitfield);
typedef void (APIENTRY * glClearDepthf_func)(GLclampf);
typedef void (APIENTRY * glClearDepth_func)(GLclampd);
typedef void (APIENTRY * glClearStencil_func)(GLint);
typedef void (APIENTRY * glDepthMask_func)(GLboolean);
typedef void (APIENTRY * glDepthFunc_func)(GLenum);
typedef void (APIENTRY * glStencilFunc_func)(GLenum, GLint, GLuint);
typedef void (APIENTRY * glStencilOp_func)(GLenum, GLenum, GLenum);
typedef void (APIENTRY * glActiveTexture_func)(GLenum texture);
typedef void (APIENTRY * glColorMask_func)(GLboolean, GLboolean, GLboolean, GLboolean);
typedef void (APIENTRY * glDeleteTextures_func)(GLsizei, const GLuint *);
typedef void (APIENTRY * glGenTextures_func)(GLsizei, GLuint *);
typedef void (APIENTRY * glBindTexture_func)(GLenum, GLuint);
typedef void (APIENTRY * glTexImage2D_func)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
typedef void (APIENTRY * glTexParameteri_func)(GLenum, GLenum, GLint);
typedef void (APIENTRY * glTextureParameteri_func)(GLuint, GLenum, GLint);
typedef void (APIENTRY * glDrawArrays_func)(GLenum, GLint, GLsizei);
typedef GLenum (APIENTRY * glGetError_func)(void);
typedef void (APIENTRY * glReadPixels_func)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
typedef void (APIENTRY * glGetIntegerv_func)(GLenum, GLint *);
typedef void (APIENTRY * glGetBooleanv_func)(GLenum, GLboolean *);
typedef GLboolean (APIENTRY * glIsEnabled_func)(GLenum);
typedef void (APIENTRY * glGenBuffers_func)(GLsizei, GLuint *);
typedef void (APIENTRY * glBindBuffer_func)(GLenum, GLuint);
typedef void (APIENTRY * glBufferData_func)(GLenum, GLsizei *, const void *, GLenum);
typedef void (APIENTRY * glDeleteBuffers_func)(GLsizei, const GLuint *);
typedef void (APIENTRY * glGenerateMipmap_func)(GLenum);
extern SHIM5_EXPORT glStencilFuncSeparate_func glStencilFuncSeparate_ptr;
extern SHIM5_EXPORT glStencilOpSeparate_func glStencilOpSeparate_ptr;
extern SHIM5_EXPORT glBindFramebuffer_func glBindFramebuffer_ptr;
extern SHIM5_EXPORT glDeleteRenderbuffers_func glDeleteRenderbuffers_ptr;
extern SHIM5_EXPORT glGenFramebuffers_func glGenFramebuffers_ptr;
extern SHIM5_EXPORT glGenRenderbuffers_func glGenRenderbuffers_ptr;
extern SHIM5_EXPORT glBindRenderbuffer_func glBindRenderbuffer_ptr;
extern SHIM5_EXPORT glFramebufferTexture2D_func glFramebufferTexture2D_ptr;
extern SHIM5_EXPORT glRenderbufferStorage_func glRenderbufferStorage_ptr;
extern SHIM5_EXPORT glCheckFramebufferStatus_func glCheckFramebufferStatus_ptr;
extern SHIM5_EXPORT glDeleteFramebuffers_func glDeleteFramebuffers_ptr;
extern SHIM5_EXPORT glFramebufferRenderbuffer_func glFramebufferRenderbuffer_ptr;
extern SHIM5_EXPORT glUseProgram_func glUseProgram_ptr;
extern SHIM5_EXPORT glUniform1f_func glUniform1f_ptr;
extern SHIM5_EXPORT glUniform2f_func glUniform2f_ptr;
extern SHIM5_EXPORT glUniform3f_func glUniform3f_ptr;
extern SHIM5_EXPORT glUniform4f_func glUniform4f_ptr;
extern SHIM5_EXPORT glUniform1i_func glUniform1i_ptr;
extern SHIM5_EXPORT glUniform2i_func glUniform2i_ptr;
extern SHIM5_EXPORT glUniform3i_func glUniform3i_ptr;
extern SHIM5_EXPORT glUniform4i_func glUniform4i_ptr;
extern SHIM5_EXPORT glUniform1fv_func glUniform1fv_ptr;
extern SHIM5_EXPORT glUniform2fv_func glUniform2fv_ptr;
extern SHIM5_EXPORT glUniform3fv_func glUniform3fv_ptr;
extern SHIM5_EXPORT glUniform4fv_func glUniform4fv_ptr;
extern SHIM5_EXPORT glUniform1iv_func glUniform1iv_ptr;
extern SHIM5_EXPORT glUniform2iv_func glUniform2iv_ptr;
extern SHIM5_EXPORT glUniform3iv_func glUniform3iv_ptr;
extern SHIM5_EXPORT glUniform4iv_func glUniform4iv_ptr;
extern SHIM5_EXPORT glUniformMatrix2fv_func glUniformMatrix2fv_ptr;
extern SHIM5_EXPORT glUniformMatrix3fv_func glUniformMatrix3fv_ptr;
extern SHIM5_EXPORT glUniformMatrix4fv_func glUniformMatrix4fv_ptr;
extern SHIM5_EXPORT glDeleteShader_func glDeleteShader_ptr;
extern SHIM5_EXPORT glCreateProgram_func glCreateProgram_ptr;
extern SHIM5_EXPORT glDeleteProgram_func glDeleteProgram_ptr;
extern SHIM5_EXPORT glAttachShader_func glAttachShader_ptr;
extern SHIM5_EXPORT glLinkProgram_func glLinkProgram_ptr;
extern SHIM5_EXPORT glGetAttribLocation_func glGetAttribLocation_ptr;
extern SHIM5_EXPORT glGetTexImage_func glGetTexImage_ptr;
extern SHIM5_EXPORT glEnableVertexAttribArray_func glEnableVertexAttribArray_ptr;
extern SHIM5_EXPORT glVertexAttribPointer_func glVertexAttribPointer_ptr;
extern SHIM5_EXPORT glGetUniformLocation_func glGetUniformLocation_ptr;
extern SHIM5_EXPORT glShaderSource_func glShaderSource_ptr;
extern SHIM5_EXPORT glCompileShader_func glCompileShader_ptr;
extern SHIM5_EXPORT glGetShaderiv_func glGetShaderiv_ptr;
extern SHIM5_EXPORT glGetShaderInfoLog_func glGetShaderInfoLog_ptr;
extern SHIM5_EXPORT glCreateShader_func glCreateShader_ptr;
extern SHIM5_EXPORT glBlendFunc_func glBlendFunc_ptr;
extern SHIM5_EXPORT glBlendFuncSeparate_func glBlendFuncSeparate_ptr;
extern SHIM5_EXPORT glEnable_func glEnable_ptr;
extern SHIM5_EXPORT glDisable_func glDisable_ptr;
extern SHIM5_EXPORT glFrontFace_func glFrontFace_ptr;
extern SHIM5_EXPORT glCullFace_func glCullFace_ptr;
extern SHIM5_EXPORT glScissor_func glScissor_ptr;
extern SHIM5_EXPORT glViewport_func glViewport_ptr;
extern SHIM5_EXPORT glClearColor_func glClearColor_ptr;
extern SHIM5_EXPORT glClear_func glClear_ptr;
extern SHIM5_EXPORT glClearDepthf_func glClearDepthf_ptr;
extern SHIM5_EXPORT glClearDepth_func glClearDepth_ptr;
extern SHIM5_EXPORT glClearStencil_func glClearStencil_ptr;
extern SHIM5_EXPORT glDepthMask_func glDepthMask_ptr;
extern SHIM5_EXPORT glDepthFunc_func glDepthFunc_ptr;
extern SHIM5_EXPORT glStencilFunc_func glStencilFunc_ptr;
extern SHIM5_EXPORT glStencilOp_func glStencilOp_ptr;
extern SHIM5_EXPORT glStencilFuncSeparate_func glStencilFuncSeparate_ptr;
extern SHIM5_EXPORT glActiveTexture_func glActiveTexture_ptr;
extern SHIM5_EXPORT glColorMask_func glColorMask_ptr;
extern SHIM5_EXPORT glDeleteTextures_func glDeleteTextures_ptr;
extern SHIM5_EXPORT glGenTextures_func glGenTextures_ptr;
extern SHIM5_EXPORT glBindTexture_func glBindTexture_ptr;
extern SHIM5_EXPORT glTexImage2D_func glTexImage2D_ptr;
extern SHIM5_EXPORT glTexParameteri_func glTexParameteri_ptr;
extern SHIM5_EXPORT glTextureParameteri_func glTextureParameteri_ptr;
extern SHIM5_EXPORT glGetError_func glGetError_ptr;
extern SHIM5_EXPORT glDrawArrays_func glDrawArrays_ptr;
extern SHIM5_EXPORT glReadPixels_func glReadPixels_ptr;
extern SHIM5_EXPORT glGetIntegerv_func glGetIntegerv_ptr;
extern SHIM5_EXPORT glGetBooleanv_func glGetBooleanv_ptr;
extern SHIM5_EXPORT glIsEnabled_func glIsEnabled_ptr;
extern SHIM5_EXPORT glGenBuffers_func glGenBuffers_ptr;
extern SHIM5_EXPORT glBindBuffer_func glBindBuffer_ptr;
extern SHIM5_EXPORT glBufferData_func glBufferData_ptr;
extern SHIM5_EXPORT glDeleteBuffers_func glDeleteBuffers_ptr;
extern SHIM5_EXPORT glGenerateMipmap_func glGenerateMipmap_ptr;

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

#include <direct.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <tgui6/tgui6.h>
#include <tgui6/tgui6_sdl3.h>

#include "shim5/basic_types.h"

#undef MIN
#undef MAX
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// To extract strings to translate
#define TRANSLATE(text) std::string(text) // for string constants eg "text" with text in English
#define REVERSE_TRANSLATE(s) s // for std::string objects eg std::string s; with s in English
#define END

namespace noo {

namespace util {

typedef void (*Callback)(void *data);

} // End namespace util

} // End namespace noo

#endif // NOO_MAIN_H
