#ifndef NOO_MAIN_H
#define NOO_MAIN_H

#ifdef _WIN32
// Disable warnings about dll-interface. Even Microsoft admits they're superfluous
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#ifdef SHIM5_STATIC
#define SHIM5_EXPORT
#else
#ifdef SHIM5_BUILD
#define SHIM5_EXPORT __declspec(dllexport)
#else
#define SHIM5_EXPORT __declspec(dllimport)
#endif
#endif

#define EXPORT_STRUCT_ALIGN(x, n) __declspec(align(n)) struct SHIM5_EXPORT x
#define EXPORT_CLASS_ALIGN(x, n) __declspec(align(n)) class SHIM5_EXPORT x
#define ALIGN(n) __declspec(align(n)) class

#else // _WIN32
#define SHIM5_EXPORT
#define EXPORT_STRUCT_ALIGN(x, n) struct __attribute__((aligned(n))) x
#define EXPORT_CLASS_ALIGN(x, n) class __attribute__((aligned(n))) x
#define ALIGN(n) class __attribute__((aligned(n)))
#endif

#include <cctype>
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

#ifndef _WIN32
#include <sys/types.h>
#include <utime.h>
#else
#include <sys/utime.h>
#define utime _utime
#define utimbuf _utimbuf
#endif
#if !defined _WIN32
#include <sys/time.h>
#endif

#if defined SDL_PLATFORM_APPLE || defined __linux__
#define _strdup strdup
#endif

#if defined IOS || defined ANDROID || defined __EMSCRIPTEN__
#define glStencilFuncSeparate_ptr glStencilFuncSeparate
#define glStencilOpSeparate_ptr glStencilOpSeparate
#define glBindFramebuffer_ptr glBindFramebuffer
#define glDeleteRenderbuffers_ptr glDeleteRenderbuffers
#define glGenFramebuffers_ptr glGenFramebuffers
#define glGenRenderbuffers_ptr glGenRenderbuffers
#define glBindRenderbuffer_ptr glBindRenderbuffer
#define glFramebufferTexture2D_ptr glFramebufferTexture2D
#define glRenderbufferStorage_ptr glRenderbufferStorage
#define glCheckFramebufferStatus_ptr glCheckFramebufferStatus
#define glDeleteFramebuffers_ptr glDeleteFramebuffers
#define glFramebufferRenderbuffer_ptr glFramebufferRenderbuffer
#define glUseProgram_ptr glUseProgram
#define glUniform1f_ptr glUniform1f
#define glUniform2f_ptr glUniform2f
#define glUniform3f_ptr glUniform3f
#define glUniform4f_ptr glUniform4f
#define glUniform1i_ptr glUniform1i
#define glUniform2i_ptr glUniform2i
#define glUniform3i_ptr glUniform3i
#define glUniform4i_ptr glUniform4i
#define glUniform1fv_ptr glUniform1fv
#define glUniform2fv_ptr glUniform2fv
#define glUniform3fv_ptr glUniform3fv
#define glUniform4fv_ptr glUniform4fv
#define glUniform1iv_ptr glUniform1iv
#define glUniform2iv_ptr glUniform2iv
#define glUniform3iv_ptr glUniform3iv
#define glUniform4iv_ptr glUniform4iv
#define glUniformMatrix2fv_ptr glUniformMatrix2fv
#define glUniformMatrix3fv_ptr glUniformMatrix3fv
#define glUniformMatrix4fv_ptr glUniformMatrix4fv
#define glDeleteShader_ptr glDeleteShader
#define glCreateProgram_ptr glCreateProgram
#define glDeleteProgram_ptr glDeleteProgram
#define glAttachShader_ptr glAttachShader
#define glLinkProgram_ptr glLinkProgram
#define glGetAttribLocation_ptr glGetAttribLocation
#define glGetTexImage_ptr glGetTexImage
#define glEnableVertexAttribArray_ptr glEnableVertexAttribArray
#define glVertexAttribPointer_ptr glVertexAttribPointer
#define glGetUniformLocation_ptr glGetUniformLocation
#define glShaderSource_ptr glShaderSource
#define glCompileShader_ptr glCompileShader
#define glGetShaderiv_ptr glGetShaderiv
#define glGetShaderInfoLog_ptr glGetShaderInfoLog
#define glCreateShader_ptr glCreateShader
#define glBlendFunc_ptr glBlendFunc
#define glEnable_ptr glEnable
#define glDisable_ptr glDisable
#define glFrontFace_ptr glFrontFace
#define glCullFace_ptr glCullFace
#define glScissor_ptr glScissor
#define glViewport_ptr glViewport
#define glClearColor_ptr glClearColor
#define glClear_ptr glClear
#define glClearDepthf_ptr glClearDepthf
#define glClearDepth_ptr glClearDepth
#define glClearStencil_ptr glClearStencil
#define glDepthMask_ptr glDepthMask
#define glDepthFunc_ptr glDepthFunc
#define glStencilFunc_ptr glStencilFunc
#define glStencilOp_ptr glStencilOp
#define glStencilFuncSeparate_ptr glStencilFuncSeparate
#define glActiveTexture_ptr glActiveTexture
#define glColorMask_ptr glColorMask
#define glDeleteTextures_ptr glDeleteTextures
#define glGenTextures_ptr glGenTextures
#define glBindTexture_ptr glBindTexture
#define glTexImage2D_ptr glTexImage2D
#define glTexParameteri_ptr glTexParameteri
#define glGetError_ptr glGetError
#define glDrawArrays_ptr glDrawArrays
#define glReadPixels_ptr glReadPixels
#define glGetIntegerv_ptr glGetIntegerv
#define glGenVertexArrays_ptr glGenVertexArrays
#define glGenBuffers_ptr glGenBuffers
#define glBindVertexArray_ptr glBindVertexArray
#define glBindBuffer_ptr glBindBuffer
#define glBufferData_ptr glBufferData
#endif

#ifdef IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#elif defined ANDROID || defined __EMSCRIPTEN__
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#ifdef SDL_PLATFORM_APPLE
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <windows.h>
/*
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_INCR_WRAP                      0x8507
#define GL_DECR_WRAP                      0x8508
#define GL_TEXTURE0                       0x84C0
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
*/
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
// OpenGL extensions (ES already has these)
#ifdef SDL_PLATFORM_APPLE
#define APIENTRY
#endif
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
typedef void (APIENTRY * glEnable_func)(GLenum);
typedef void (APIENTRY * glDisable_func)(GLenum);
typedef void (APIENTRY * glFrontFace_func)(GLenum);
typedef void (APIENTRY * glCullFace_func)(GLenum);
typedef void (APIENTRY * glScissor_func)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY * glViewport_func)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY * glClearColor_func)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (APIENTRY * glClear_func)(GLbitfield);
typedef void (APIENTRY * glClearDepthf_func)(GLclampf);
#if !defined ANDROID && !defined IOS
typedef void (APIENTRY * glClearDepth_func)(GLclampd);
#endif
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
typedef void (APIENTRY * glDrawArrays_func)(GLenum, GLint, GLsizei);
typedef GLenum (APIENTRY * glGetError_func)(void);
typedef void (APIENTRY * glReadPixels_func)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
typedef void (APIENTRY * glGetIntegerv_func)(GLenum, GLint *);
typedef void (APIENTRY * glGenVertexArrays_func)(GLsizei, GLuint *);
typedef void (APIENTRY * glGenBuffers_func)(GLsizei, GLuint *);
typedef void (APIENTRY * glBindVertexArray_func)(GLuint);
typedef void (APIENTRY * glBindBuffer_func)(GLenum, GLuint);
typedef void (APIENTRY * glBufferData_func)(GLenum, GLsizei *, const void *, GLenum);
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
extern SHIM5_EXPORT glEnable_func glEnable_ptr;
extern SHIM5_EXPORT glDisable_func glDisable_ptr;
extern SHIM5_EXPORT glFrontFace_func glFrontFace_ptr;
extern SHIM5_EXPORT glCullFace_func glCullFace_ptr;
extern SHIM5_EXPORT glScissor_func glScissor_ptr;
extern SHIM5_EXPORT glViewport_func glViewport_ptr;
extern SHIM5_EXPORT glClearColor_func glClearColor_ptr;
extern SHIM5_EXPORT glClear_func glClear_ptr;
extern SHIM5_EXPORT glClearDepthf_func glClearDepthf_ptr;
#if !defined ANDROID && !defined IOS
extern SHIM5_EXPORT glClearDepth_func glClearDepth_ptr;
#endif
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
extern SHIM5_EXPORT glGetError_func glGetError_ptr;
extern SHIM5_EXPORT glDrawArrays_func glDrawArrays_ptr;
extern SHIM5_EXPORT glReadPixels_func glReadPixels_ptr;
extern SHIM5_EXPORT glGetIntegerv_func glGetIntegerv_ptr;
extern SHIM5_EXPORT glGenVertexArrays_func glGenVertexArrays_ptr;
extern SHIM5_EXPORT glGenBuffers_func glGenBuffers_ptr;
extern SHIM5_EXPORT glBindVertexArray_func glBindVertexArray_ptr;
extern SHIM5_EXPORT glBindBuffer_func glBindBuffer_ptr;
extern SHIM5_EXPORT glBufferData_func glBufferData_ptr;
#endif

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

#if defined _WIN32
#include <direct.h>
#else
#ifndef ANDROID
#include <glob.h>
#endif
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined __linux__ && !defined ANDROID
#include <X11/Xcursor/Xcursor.h>
#endif
#endif

#ifdef SDL2
#if __linux__ || (defined SDL_PLATFORM_APPLE) || defined __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif
#ifdef USE_TTF
#ifdef __EMSCRIPTEN__
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#endif
#else
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#endif

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
