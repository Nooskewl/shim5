#ifndef NOO_GFX_H
#define NOO_GFX_H

#include "shim5/main.h"
#include "shim5/model.h"

namespace noo {

namespace gfx {

class Image;

enum Compare_Func {
	COMPARE_NEVER = 1,
	COMPARE_LESS,
	COMPARE_EQUAL,
	COMPARE_LESSEQUAL,
	COMPARE_GREATER,
	COMPARE_NOTEQUAL,
	COMPARE_GREATEREQUAL,
	COMPARE_ALWAYS
};

enum Stencil_Op {
	STENCILOP_KEEP = 1,
	STENCILOP_ZERO,
	STENCILOP_REPLACE,
	STENCILOP_INCRSAT,
	STENCILOP_DECRSAT,
	STENCILOP_INVERT,
	STENCILOP_INCR,
	STENCILOP_DECR
};

enum Front_Face {
	FACE_CW,
	FACE_CCW
};

enum Faces {
	NO_FACE = 0,
	FRONT_FACE,
	BACK_FACE
};

enum Blend_Mode {
	BLEND_ZERO = 0,
	BLEND_ONE,
	BLEND_SRCCOLOR,
	BLEND_INVSRCCOLOR,
	BLEND_SRCALPHA,
	BLEND_INVSRCALPHA
};

// See comments in shim.h for what these parameters are.
bool SHIM5_EXPORT static_start();
void SHIM5_EXPORT static_end();

bool SHIM5_EXPORT start(int scaled_w = -1, int scaled_h = -1, bool force_integer_scaling = false, int window_w = -1, int window_h = -1);
bool SHIM5_EXPORT restart(int scaled_w = -1, int scaled_h = -1, bool force_integer_scaling = false, int window_w = -1, int window_h = -1);
void SHIM5_EXPORT end();

void SHIM5_EXPORT update_animations();

void SHIM5_EXPORT draw_guis();
void SHIM5_EXPORT draw_notifications();
void SHIM5_EXPORT flip();

void SHIM5_EXPORT clear(SDL_Color colour);
void SHIM5_EXPORT clear_depth_buffer(float value);
void SHIM5_EXPORT clear_stencil_buffer(int value);
void SHIM5_EXPORT clear_buffers();

void SHIM5_EXPORT enable_depth_test(bool onoff);
void SHIM5_EXPORT enable_depth_write(bool onoff);
bool SHIM5_EXPORT is_depth_test_enabled();
bool SHIM5_EXPORT is_depth_write_enabled();
void SHIM5_EXPORT set_depth_mode(Compare_Func func);
Compare_Func SHIM5_EXPORT get_depth_mode();

void SHIM5_EXPORT enable_stencil(bool onoff);
void SHIM5_EXPORT enable_two_sided_stencil(bool onoff);
bool SHIM5_EXPORT is_stencil_enabled();
bool SHIM5_EXPORT is_two_sided_stencil_enabled();
void SHIM5_EXPORT set_stencil_mode(Compare_Func func, Stencil_Op fail, Stencil_Op zfail, Stencil_Op pass, int reference, int mask);
void SHIM5_EXPORT set_stencil_mode_backfaces(Compare_Func func, Stencil_Op fail, Stencil_Op zfail, Stencil_Op pass, int reference, int mask);

void SHIM5_EXPORT set_front_face(Front_Face face);
void SHIM5_EXPORT set_cull_mode(Faces cull);

void SHIM5_EXPORT enable_blending(bool onoff);
void SHIM5_EXPORT set_blend_mode(Blend_Mode source, Blend_Mode dest);
bool SHIM5_EXPORT is_blending_enabled();
void SHIM5_EXPORT get_blend_mode(Blend_Mode &src, Blend_Mode &dst);

void SHIM5_EXPORT enable_colour_write(bool onoff);
bool SHIM5_EXPORT is_colour_write_enabled();

void SHIM5_EXPORT set_default_projection(util::Size<int> screen_size, util::Point<int> screen_offset, float scale);
void SHIM5_EXPORT get_matrices(glm::mat4 &modelview, glm::mat4 &proj);
void SHIM5_EXPORT set_matrices(glm::mat4 &modelview, glm::mat4 &proj);
void SHIM5_EXPORT update_projection();

void SHIM5_EXPORT set_scissor(int x, int y, int w, int h);
void SHIM5_EXPORT unset_scissor();
void SHIM5_EXPORT get_scissor(int &x, int &y, int &w, int &h);

void SHIM5_EXPORT set_viewport(int x, int y, int w, int h);
void SHIM5_EXPORT unset_viewport();
void SHIM5_EXPORT get_viewport(int &x, int &y, int &w, int &h);

Image SHIM5_EXPORT *get_target_image();
void SHIM5_EXPORT set_target_image(Image *image);
void SHIM5_EXPORT set_target_backbuffer();

// warning: lost can be called multiple times consecutively without a found (if delete'ing stuff, set it to NULL)
typedef void (*_lost_device_callback)(void);
typedef void (**_lost_device_callback_pointer)(void);
void SHIM5_EXPORT register_lost_device_callbacks(_lost_device_callback, _lost_device_callback);
void SHIM5_EXPORT get_lost_device_callbacks(_lost_device_callback_pointer, _lost_device_callback_pointer);

void SHIM5_EXPORT reload_fonts();

void SHIM5_EXPORT set_minimum_window_size(util::Size<int> size);
void SHIM5_EXPORT set_maximum_window_size(util::Size<int> size);
void SHIM5_EXPORT set_min_aspect_ratio(float min); // anything over/under gets black bars. default: 4.0f / 3.0f.
void SHIM5_EXPORT set_max_aspect_ratio(float max); // anything over/under gets black bars. default: 16.0f / 9.0f.

util::Size<int> SHIM5_EXPORT get_desktop_resolution();
std::vector< util::Size<int> > SHIM5_EXPORT get_supported_video_modes();

void SHIM5_EXPORT show_mouse_cursor(bool show);

int SHIM5_EXPORT load_palette(std::string name, SDL_Color *out, int out_size = 256);
int SHIM5_EXPORT load_default_palette();

void SHIM5_EXPORT add_notification(std::string text);
std::string SHIM5_EXPORT get_current_notification();
void SHIM5_EXPORT cancel_current_notification();
void SHIM5_EXPORT cancel_all_notifications();

void SHIM5_EXPORT draw_9patch_tinted(SDL_Color tint, Image *image, util::Point<float> dest_position, util::Size<int> dest_size);
void SHIM5_EXPORT draw_9patch(Image *image, util::Point<int> dest_position, util::Size<int> dest_size);

void SHIM5_EXPORT reset_fancy_draw();
void SHIM5_EXPORT fancy_draw(SDL_Color colour, std::string text, util::Point<int> position);

void SHIM5_EXPORT set_custom_mouse_cursor(); // can be needed in some cases but rarely

void SHIM5_EXPORT set_scaled_size(util::Size<int> size);
void SHIM5_EXPORT set_screen_size(util::Size<int> size);

int SHIM5_EXPORT get_max_comfortable_scale(util::Size<int> scaled_size);

bool SHIM5_EXPORT enable_press_and_hold(bool enable);

bool SHIM5_EXPORT is_fullscreen();
bool SHIM5_EXPORT is_real_fullscreen();
bool SHIM5_EXPORT is_fullscreen_window();

void SHIM5_EXPORT resize_window(int width, int height);

gfx::Image SHIM5_EXPORT *gen_plasma(int seed, float alpha1, float alpha2, SDL_Color tint);

void SHIM5_EXPORT screen_shake(float amount, Uint32 length);
void SHIM5_EXPORT apply_screen_shake();

enum Black_Bar_Type {
	BAR_TOP,
	BAR_BOTTOM,
	BAR_LEFT,
	BAR_RIGHT
};

typedef void (*_black_bars_callback)(Black_Bar_Type type, int x, int y, int w, int h);
void SHIM5_EXPORT register_black_bars_callback(_black_bars_callback);

#if ((defined SDL_PLATFORM_APPLE && !defined IOS) || (defined __linux__ && !defined ANDROID && !defined RASPBERRYPI) || defined _WIN32 || defined __EMSCRIPTEN__)
void SHIM5_EXPORT create_mouse_cursors();
void SHIM5_EXPORT delete_mouse_cursors();
#endif

} // End namespace gfx

} // End namespace noo

#endif // NOO_GFX_H
