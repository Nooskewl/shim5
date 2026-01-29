#include "shim5/cpa.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/vertex_cache.h"
#include "libutil/libutil.h"
#include "libutil/internal.h"

#ifdef SDL_PLATFORM_APPLE
#include "shim5/apple.h"
#ifdef IOS
#include "shim5/ios.h"
#else
#include "shim5/macosx.h"
#endif
#endif

#include <sys/stat.h>

#ifdef _WIN32
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
#include "shim5/steamworks.h"
#endif

#include "shim5/internal/gfx.h"
#include "shim5/internal/util.h"

namespace noo {

namespace util {

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
#ifdef _WIN32
	if (gfx::internal::gfx_context.fullscreen) {
		ShowWindow(gfx::internal::gfx_context.hwnd, SW_MINIMIZE);
	}
	ShellExecute(0, 0, filename.c_str(), 0, 0 , SW_SHOW);
#elif defined __linux__
	pid_t pid = fork();
	if (pid == 0) {
		system((std::string("xdg-open ") + filename).c_str());
		exit(0);
	}
#elif !defined IOS && !defined __EMSCRIPTEN__
	macosx_open_with_system(filename);
#endif
}

void open_url(std::string url)
{
#ifdef ANDROID
	JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
	jclass clazz(env->GetObjectClass(activity));

	jstring S = env->NewStringUTF(url.c_str());

	jmethodID method_id = env->GetMethodID(clazz, "openURL", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, method_id, S);

	env->DeleteLocalRef(S);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
#elif defined __linux__
	// FIXME: might work on win/mac too
	open_with_system(url);
#endif
}

} // end namespace util

} // end namespace noo
