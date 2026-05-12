#ifndef NOO_I_GFX_H
#define NOO_I_GFX_H

#include "shim5/main.h"

namespace noo {

namespace gfx {

class Image;
class Shader;

namespace internal {

struct GFX_Context {
	bool inited;
	bool fullscreen;
	bool fullscreen_window;
	Image *target_image;
	SDL_Window *window;
	Uint32 windowid;
	bool restarting;
	SDL_GLContext opengl_context;
	SDL_Mutex *draw_mutex;
	bool mouse_in_window;
	Shader *textured_shader;
	Shader *untextured_shader;
	HWND hwnd;
};

extern SHIM5_EXPORT GFX_Context gfx_context;

bool scale_mouse_event(TGUI_Event *event);
void handle_lost_device(bool including_opengl, bool force = false);
void handle_found_device(bool including_opengl, bool force = false);
int My_SDL_GetCurrentDisplayMode(int adapter, SDL_DisplayMode *mode);

HICON win_create_icon(HWND wnd, Uint8 *data, util::Size<int> size, int xfocus, int yfocus, bool is_cursor);

void premultiply_surface(SDL_Surface *surface);

} // End namespace internal

} // End namespace gfx

} // End namespace noo

#endif // NOO_I_GFX_H
