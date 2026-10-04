#include "shim5/main.h"
#include "shim5/audio.h"
#include "shim5/cpa.h"
#include "shim5/devsettings.h"
#include "shim5/gfx.h"
#include "shim5/gui.h"
#include "shim5/image.h"
#include "shim5/input.h"
#include "shim5/json.h"
#include "shim5/mml.h"
#include "shim5/primitives.h"
#include "shim5/sample.h"
#include "shim5/shim.h"
#include "shim5/sprite.h"
#include "shim5/translation.h"
#include "shim5/util.h"
#include "shim5/vertex_cache.h"
#include "shim5/util.h"

#include "shim5/internal/audio.h"
#include "shim5/internal/gfx.h"
#include "shim5/internal/shim.h"
#include "shim5/internal/util.h"

using namespace noo;

namespace noo {

namespace shim {

static TGUI_Event *tgui_event;
static input::Focus_Event *focus_event;
static std::vector<TGUI_Event> pushed_events;
static bool waiting_for_fullscreen_change;
static bool waiting_for_resize;
static bool quitting;
static util::Size<int> switch_out_screen_size;
static Uint32 last_mouse_event;
static bool joystick_was_connected;
static bool joy_lb;
static bool joy_rb;

SDL_Color palette[256];
int palette_size;
SDL_Color black;
SDL_Color white;
SDL_Color magenta;
SDL_Color transparent;
SDL_Color interface_bg;
SDL_Color interface_highlight;
SDL_Color interface_text;
SDL_Color interface_edit_fg;
SDL_Color interface_edit_bg;
std::vector<gui::GUI *> guis;
float scale;
std::string window_title;
gfx::Shader *current_shader;
gfx::Shader *default_shader;
gfx::Shader *model_shader;
int tile_size;
util::Size<int> screen_size;
util::Size<int> real_screen_size;
gfx::Font *font;
int font_size;
std::string font_name;
bool create_depth_buffer;
bool create_stencil_buffer;
util::Size<int> depth_buffer_size;
util::Point<int> screen_offset;
float letterbox_percent;
float z_add;
//void (*user_render)();
int refresh_rate;
bool hide_window;
util::Point<int> cursor_hotspot;
bool scale_mouse_cursor;
bool multisampling;
int aa_samples;
audio::MML *widget_sfx;
audio::MML *button_sfx;
int key_l;
int key_r;
int key_u;
int key_d;
int fullscreen_key;
bool linear_filtering;
int devsettings_key;
int screenshot_key;
bool take_screenshot;
bool convert_directions_to_focus_events;
float joystick_activate_threshold;
float joystick_deactivate_threshold;
bool mouse_button_repeats;
int mouse_button_repeat_max_movement;
bool dpad_enabled;
void (*joystick_disconnect_callback)();
bool force_tablet;
int notification_duration;
int notification_fade_duration;
Uint32 timer_event_id;
int logic_rate;
int devsettings_num_rows;
int devsettings_max_width;
std::vector<util::A_Star::Way_Point> (*get_way_points)(util::Point<int> start);
util::Point<float> screen_shake_save;
bool using_screen_shake;
std::string organisation_name;
std::string game_name;
int argc;
char **argv;
bool debug;
util::CPA *cpa;
util::CPA *default_cpa;
int cpa_extra_bytes_after_exe_data;
Uint8 *cpa_pointer_to_data;
int cpa_data_size;
bool logging;
bool use_cwd;
bool log_tags;
#ifdef DEBUG
int error_level = 9999;
#else
int error_level = 1;
#endif
util::JSON *shim_json;
gui::GUI::Transition_Type gui_transition_in_type = gui::GUI::TRANSITION_SHRINK;
gui::GUI::Transition_Type gui_transition_out_type = gui::GUI::TRANSITION_SHRINK;

static void handle_resize(SDL_Event *event)
{
	// Right after recreating the window on Windows we can get window events from the old window
	if (event->window.windowID != gfx::internal::gfx_context.windowid) {
		return;
	}

	if (gfx::internal::gfx_context.fullscreen == false) {
		gfx::resize_window(event->window.data1, event->window.data2);
	}
}

// this may run in a different thread :/
static bool event_filter(void *userdata, SDL_Event *event)
{
	switch (event->type)
	{
		default:
			return 1;
	}
}

static bool init_sdl(int sdl_init_flags)
{
	if (SDL_Init(sdl_init_flags) != true) {
		throw util::Error(util::string_printf("SDL_Init failed: %s.", SDL_GetError()));
		return false;
	}
	
	SDL_SetEventFilter(event_filter, NULL);

	return true;
}

static void load_mml()
{
	widget_sfx = nullptr;
	button_sfx = nullptr;

	try {
		Uint8 *bytes;
		SDL_IOStream *file;
		std::string str;
		str = "@PO0 = { 0 50 }\n@VAS0 = { 0 255 255 255 0 }\nA @TYPE3 >> @PO0 @VAS0 g64 @VAS0 @PO0";
		bytes = (Uint8 *)str.c_str();
		file = SDL_IOFromMem(bytes, str.length());
		widget_sfx = new audio::MML(file); // this closes the file
		str = "@PO0 = { 0 -25 }\n@VAS0 = { 0 }\nA @TYPE3 @PO0 @VAS0 g80 @VAS0 @PO0";
		bytes = (Uint8 *)str.c_str();
		file = SDL_IOFromMem(bytes, str.length());
		button_sfx = new audio::MML(file); // this closes the file
	}
	catch (util::Error &e) {
		util::infomsg(e.error_message + "\n");
	}
}

static void destroy_mml()
{
	delete widget_sfx;
	delete button_sfx;
}

bool static_start(int sdl_init_flags)
{
	if (util::basic_start() == false) {
		return false;
	}

	if (sdl_init_flags == 0) {
		sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;
	}


	if (init_sdl(sdl_init_flags) == false) {
		return false;
	}

#if 0
	try {
		if (cpa_pointer_to_data != 0) {
			cpa = new util::CPA(cpa_pointer_to_data, cpa_data_size);
		}
		/*
		else if (argv != 0) {
			cpa = new util::CPA(argv[0]);
		}
		*/
		else {
			throw util::Error("Catch me!");
		}
	}
	catch (util::Error &e) {
#endif
		try {
			cpa = new util::CPA();
		}
		catch (util::Error &e) {
			util::infomsg(e.error_message + "\n");
		}

	default_cpa = cpa;

	// argc/argv should be set before static_start in shim5 (util::static_start uses them)
	//argc = 0;
	//argv = 0;

	tgui_event = 0;
	focus_event = 0;
	pushed_events.clear();
	waiting_for_fullscreen_change = false;
	waiting_for_resize = false;
	letterbox_percent = 0.0f;
	z_add = 0.0f;
	//user_render = 0;
	refresh_rate = 0;
	widget_sfx = 0;
	button_sfx = 0;
	cpa_extra_bytes_after_exe_data = 0;
	cpa_pointer_to_data = 0;
	cpa_data_size = 0;
	timer_event_id = (Uint32)-1;
	quitting = false;
	font = 0;
	font_size = 8;
	font_name = "font.ttf";
	cursor_hotspot = {0, 0};
	guis.clear();
	last_mouse_event = SDL_GetTicks();
	joystick_was_connected = false;
	joystick_disconnect_callback = nullptr;
	
	switch_out_screen_size = {-1, -1};

	try {
		shim_json = new util::JSON("shim5.json", false);
	}
	catch (util::Error &e) {
		try {
			shim_json = new util::JSON("shim5.json", true);
		}
		catch (util::Error &e) {
			// Use a default file
			std::string json = "{}";
			SDL_IOStream *file = SDL_IOFromMem((void *)json.c_str(), 2);
			assert(file);
			shim_json = new util::JSON(file);
			SDL_CloseIO(file);
		}
	}

	util::JSON::Node *root = shim_json->get_root();

	try {
		util::JSON *user_json = new util::JSON(util::get_savegames_dir() + "/shim5.json", true);
		root->merge(user_json->get_root(), nullptr);
		delete user_json;
	}
	catch (util::Error &e) {
	}

	black.r = root->get_nested_byte("shim>gfx>colours>black.r", &black.r, 0);
	black.g = root->get_nested_byte("shim>gfx>colours>black.g", &black.g, 0);
	black.b = root->get_nested_byte("shim>gfx>colours>black.b", &black.b, 0);
	black.a = root->get_nested_byte("shim>gfx>colours>black.a", &black.a, 255);
	white.r = root->get_nested_byte("shim>gfx>colours>white.r", &white.r, 255);
	white.g = root->get_nested_byte("shim>gfx>colours>white.g", &white.g, 255);
	white.b = root->get_nested_byte("shim>gfx>colours>white.b", &white.b, 255);
	white.a = root->get_nested_byte("shim>gfx>colours>white.a", &white.a, 255);
	magenta.r = root->get_nested_byte("shim>gfx>colours>magenta.r", &magenta.r, 255);
	magenta.g = root->get_nested_byte("shim>gfx>colours>magenta.g", &magenta.g, 0);
	magenta.b = root->get_nested_byte("shim>gfx>colours>magenta.b", &magenta.b, 255);
	magenta.a = root->get_nested_byte("shim>gfx>colours>magenta.a", &magenta.a, 255);
	transparent.r = root->get_nested_byte("shim>gfx>colours>transparent.r", &transparent.r, 0);
	transparent.g = root->get_nested_byte("shim>gfx>colours>transparent.g", &transparent.g, 0);
	transparent.b = root->get_nested_byte("shim>gfx>colours>transparent.b", &transparent.b, 0);
	transparent.a = root->get_nested_byte("shim>gfx>colours>transparent.a", &transparent.a, 0);
	interface_bg.r = root->get_nested_byte("shim>gfx>colours>interface_bg.r", &interface_bg.r, 50);
	interface_bg.g = root->get_nested_byte("shim>gfx>colours>interface_bg.g", &interface_bg.g, 150);
	interface_bg.b = root->get_nested_byte("shim>gfx>colours>interface_bg.b", &interface_bg.b, 200);
	interface_bg.a = root->get_nested_byte("shim>gfx>colours>interface_bg.a", &interface_bg.a, 255);
	interface_highlight.r = root->get_nested_byte("shim>gfx>colours>interface_highlight.r", &interface_highlight.r, 50);
	interface_highlight.g = root->get_nested_byte("shim>gfx>colours>interface_highlight.g", &interface_highlight.g, 200);
	interface_highlight.b = root->get_nested_byte("shim>gfx>colours>interface_highlight.b", &interface_highlight.b, 250);
	interface_highlight.a = root->get_nested_byte("shim>gfx>colours>interface_highlight.a", &interface_highlight.a, 255);
	interface_text.r = root->get_nested_byte("shim>gfx>colours>interface_text.r", &interface_text.r, 255);
	interface_text.g = root->get_nested_byte("shim>gfx>colours>interface_text.g", &interface_text.g, 255);
	interface_text.b = root->get_nested_byte("shim>gfx>colours>interface_text.b", &interface_text.b, 255);
	interface_text.a = root->get_nested_byte("shim>gfx>colours>interface_text.a", &interface_text.a, 255);
	interface_edit_fg.r = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.r", &interface_edit_fg.r, 0);
	interface_edit_fg.g = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.g", &interface_edit_fg.g, 0);
	interface_edit_fg.b = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.b", &interface_edit_fg.b, 0);
	interface_edit_fg.a = root->get_nested_byte("shim>gfx>colours>interface_edit_fg.a", &interface_edit_fg.a, 255);
	interface_edit_bg.r = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.r", &interface_edit_bg.r, 255);
	interface_edit_bg.g = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.g", &interface_edit_bg.g, 255);
	interface_edit_bg.b = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.b", &interface_edit_bg.b, 255);
	interface_edit_bg.a = root->get_nested_byte("shim>gfx>colours>interface_edit_bg.a", &interface_edit_bg.a, 255);
	window_title = root->get_nested_string("shim>misc>window_title", &window_title, window_title, true, true);
	organisation_name = root->get_nested_string("shim>misc>organisation_name", &organisation_name, organisation_name, true, true);
	game_name = root->get_nested_string("shim>misc>game_name", &game_name, game_name, true, true);
	create_depth_buffer = root->get_nested_bool("shim>gfx>create_depth_buffer", &create_depth_buffer, false);
	create_stencil_buffer = root->get_nested_bool("shim>gfx>create_stencil_buffer", &create_stencil_buffer, false);
	depth_buffer_size.w = root->get_nested_int("shim>gfx>depth_buffer_size.w", &depth_buffer_size.w, -1);
	depth_buffer_size.h = root->get_nested_int("shim>gfx>depth_buffer_size.h", &depth_buffer_size.h, -1);
	scale_mouse_cursor = root->get_nested_bool("shim>gfx>scale_mouse_cursor", &scale_mouse_cursor, false, true, false);
	hide_window = root->get_nested_bool("shim>gfx>hide_window", &hide_window, false, true, true);
	// these are for devsettings
	key_l = TGUIK_LEFT;
	key_r = TGUIK_RIGHT;
	key_u = TGUIK_UP;
	key_d = TGUIK_DOWN;
	fullscreen_key = root->get_nested_int("shim>input>fullscreen_key", &fullscreen_key, TGUIK_F11);
	devsettings_key = root->get_nested_int("shim>input>devsettings_key", &devsettings_key, TGUIK_F9);
	screenshot_key = root->get_nested_int("shim>input>screenshot_key", &screenshot_key, TGUIK_F10);
	take_screenshot = false;
	convert_directions_to_focus_events = root->get_nested_bool("shim>input>convert_directions_to_focus_events", &convert_directions_to_focus_events, true);
	mouse_button_repeats = root->get_nested_bool("shim>input>mouse_button_repeats", &mouse_button_repeats, true);
	mouse_button_repeat_max_movement = root->get_nested_int("shim>input>mouse_button_repeat_max_movement", &mouse_button_repeat_max_movement, -1); // * shim::scale
	force_tablet = root->get_nested_bool("shim>input>force_tablet", &force_tablet, false, true, false);
	notification_duration = root->get_nested_int("shim>gfx>notification_duration", &notification_duration, 3000);
	notification_fade_duration = root->get_nested_int("shim>gfx>notification_fade_duration", &notification_fade_duration, 500);
	logging = root->get_nested_bool("shim>misc>logging", &logging, logging, true, true);
	logic_rate = root->get_nested_int("shim>misc>logic_rate", &logic_rate, 60);
	use_cwd = root->get_nested_bool("shim>misc>use_cwd", &use_cwd, false);
	log_tags = root->get_nested_bool("shim>misc>log_tags", &log_tags, true);
	error_level = root->get_nested_int("shim>misc>error_level", &error_level, error_level);
	tile_size = root->get_nested_int("shim>gfx>tile_size", &tile_size, 16);
	devsettings_num_rows = root->get_nested_int("shim>misc>devsettings_num_rows", &devsettings_num_rows, 6);
	devsettings_max_width = root->get_nested_int("shim>misc>devsettings_max_width", &devsettings_max_width, 100);
	cursor_hotspot.x = root->get_nested_int("shim>gfx>cursor_hotspot.x", &shim::cursor_hotspot.x, 0);
	cursor_hotspot.y = root->get_nested_int("shim>gfx>cursor_hotspot.y", &shim::cursor_hotspot.y, 0);
	
	debug = root->get_nested_bool("shim>misc>debug", &debug, false);
	linear_filtering = root->get_nested_bool("shim>gfx>linear_filtering", &linear_filtering, false, true, true);

	get_way_points = nullptr;

	return true;
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS *ExceptionInfo)
{
	util::flush_log_file();
	util::close_log_file();
	return EXCEPTION_EXECUTE_HANDLER;
}

bool static_start_all(int sdl_init_flags)
{
	if (static_start(sdl_init_flags) == false) {
		return false;
	}
	if (util::static_start() == false) {
		return false;
	}

	SetUnhandledExceptionFilter(CrashHandler);

	if (audio::static_start() == false) {
		return false;
	}
	if (gfx::static_start() == false) {
		return false;
	}

	return true;
}

bool start()
{
	util::JSON::Node *root = shim::shim_json->get_root();
	debug = root->get_nested_bool("shim>misc>debug", &debug, false, true, true);
	debug = util::bool_arg(debug, shim::argc, shim::argv, "debug");
	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+error-level")) >= 0) {
		shim::error_level = atoi(shim::argv[index+1]);
	}

	force_tablet = force_tablet || util::bool_arg(false, shim::argc, shim::argv, "force-tablet");

	linear_filtering = util::bool_arg(linear_filtering, shim::argc, shim::argv, "linear-filtering");

	if (timer_event_id == (Uint32)-1) {
		timer_event_id = SDL_RegisterEvents(1);
	}

	// Load resources

	load_mml();

	tgui_event = new TGUI_Event;
	focus_event = new input::Focus_Event;

	return true;
}

bool start_all(int scaled_gfx_w, int scaled_gfx_h, bool force_integer_scaling, int gfx_window_w, int gfx_window_h)
{
	util::start();

	if (audio::start() == false) {
		/*
		delete cpa;
		return false;
		*/
		// should continue muted
	}
	
	start();

	if (gfx::start(scaled_gfx_w, scaled_gfx_h, force_integer_scaling, gfx_window_w, gfx_window_h) == false) {
		audio::end();
		delete cpa;
		throw util::Error(util::string_printf("gfx::start failed."));
		return false;
	}

	if (input::start() == false) {
		gfx::end();
		audio::end();
		delete cpa;
		throw util::Error(util::string_printf("input::start failed."));
		return false;
	}

	return true;
}

void static_end()
{
	delete shim_json;
	SDL_Quit();
}

void static_end_all()
{
	gfx::static_end();
	static_end();
	util::static_end();
}

void end()
{
	for (size_t i = 0; i < guis.size(); i++) {
		delete guis[i];
	}

	destroy_mml();

	delete cpa;

	delete tgui_event;
	delete focus_event;

	util::infomsg("%d unfreed images.\n", gfx::Image::get_unfreed_count());
}

void end_all()
{
	input::end();

	gfx::end();

	end();

	audio::end();

	util::end();
}

static TGUI_Event *real_handle_tgui_event(TGUI_Event *tgui_event)
{
	if (tgui_event->type == TGUI_UNKNOWN) {
		return tgui_event;
	}

	if (tgui_event->type == TGUI_MOUSE_AXIS || tgui_event->type == TGUI_MOUSE_DOWN || tgui_event->type == TGUI_MOUSE_UP) {
		last_mouse_event = SDL_GetTicks();
		gfx::show_mouse_cursor(true);
		gfx::set_custom_mouse_cursor();
	}
	else {
		Uint32 now = SDL_GetTicks();
		Uint32 elapsed = now - last_mouse_event;
		if (elapsed > 10000 && input::is_joystick_connected()) {
			gfx::show_mouse_cursor(false);
		}
	}

	bool ret = gfx::internal::scale_mouse_event(tgui_event);

	input::handle_event(tgui_event);

	bool is_focus;

	if (convert_directions_to_focus_events) {
		is_focus = input::convert_to_focus_event(tgui_event, focus_event);
	}
	else {
		is_focus = false;
	}

	TGUI_Event *event;
	if (is_focus) {
		event = focus_event;
	}
	else {
		event = tgui_event;
	}

	if (ret) {
		return event;
	}

	if (guis.size() > 0) {
		gui::GUI *noo_gui = guis[guis.size()-1];
		if (noo_gui->is_transitioning_out() == false) {
			noo_gui->handle_event(event);
		}
	}

	if (!gfx::internal::gfx_context.fullscreen && event->type == TGUI_KEY_DOWN && event->keyboard.code == fullscreen_key && event->keyboard.is_repeat == false) {
		if (shim::guis.size() == 0 || dynamic_cast<gui::DevSettings_GUI *>(shim::guis.back()) == NULL) {
			waiting_for_fullscreen_change = true;
			gfx::clear(shim::black);
			gfx::flip();
			gfx::internal::gfx_context.fullscreen_window = !gfx::internal::gfx_context.fullscreen_window;
			SDL_SetWindowFullscreen(gfx::internal::gfx_context.window, gfx::internal::gfx_context.fullscreen_window ? true : false);
			gfx::clear(shim::black);
			gfx::flip();
		}
	}

	if (event->type == TGUI_JOY_DOWN && event->joystick.is_repeat == false) {
		if (event->joystick.button == TGUI_B_LB) {
			joy_lb = true;
		}
		else if (event->joystick.button == TGUI_B_RB) {
			joy_rb = true;
		}
	}
	else if (event->type == TGUI_JOY_UP && event->joystick.is_repeat == false) {
		if (event->joystick.button == TGUI_B_LB) {
			joy_lb = false;
		}
		else if (event->joystick.button == TGUI_B_RB) {
			joy_rb = false;
		}
	}

	if ((event->type == TGUI_KEY_DOWN && event->keyboard.code == devsettings_key && event->keyboard.is_repeat == false) || (joy_lb && joy_rb)) {
#ifdef DEBUG
		if (true) {
#else
		if (shim::debug) {
#endif
			if (shim::guis.size() == 0 || dynamic_cast<gui::DevSettings_GUI *>(shim::guis.back()) == NULL) {
				gui::DevSettings_GUI *gui = new gui::DevSettings_GUI();
				shim::guis.push_back(gui);
			}
			// exiting is done in the gui itself now
			/*
			else {
				gui::GUI *gui = shim::guis.back();
				gui::DevSettings_GUI *d = dynamic_cast<gui::DevSettings_GUI *>(gui);
				if (d->is_editing() == false) {
					shim::guis.pop_back();
					delete gui;
				}
			}
			*/
		}
	}
	
	if (event->type == TGUI_KEY_DOWN && event->keyboard.code == screenshot_key && event->keyboard.is_repeat == false) {
		take_screenshot = true;
	}

	// Don't pass events to game if DevSettings_GUI is up
	bool found_devsettings = false;
	for (auto g : shim::guis) {
		if (dynamic_cast<gui::DevSettings_GUI *>(g)) {
			found_devsettings = true;
			break;
		}
	}
	if (found_devsettings) {
		event->type = TGUI_UNKNOWN;
	}

	return event;
}

TGUI_Event *handle_event(SDL_Event *sdl_event)
{
	if (sdl_event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
		waiting_for_fullscreen_change = false;
		handle_resize(sdl_event);
	}
	else if (sdl_event->type == SDL_EVENT_WINDOW_MOUSE_ENTER) {
		gfx::internal::gfx_context.mouse_in_window = true;
	}
	else if (sdl_event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
		gfx::internal::gfx_context.mouse_in_window = false;
	}
	else if (sdl_event->type == SDL_EVENT_QUIT) {
		quitting = true;
	}

	if (sdl_event->type == timer_event_id) {
		tgui_event->type = TGUI_TICK;
	}
	else {
		*tgui_event = tgui_sdl_convert_event(sdl_event);
	}

	TGUI_Event *event = real_handle_tgui_event(tgui_event);

	return event;
}

TGUI_Event *pop_pushed_event()
{
	if (pushed_events.size() == 0) {
		return 0;
	}
	else {
		*tgui_event = pushed_events[0];
		pushed_events.erase(pushed_events.begin());
		return real_handle_tgui_event(tgui_event);
	}
}

/*
bool event_in_queue(TGUI_Event e)
{
	for (size_t i = 0; i < pushed_events.size(); i++) {
		TGUI_Event &e2 = pushed_events[i];
		if (e.type == e2.type) {
			switch (e.type) {
				case TGUI_UNKNOWN:
				case TGUI_QUIT:
					return true;
				case TGUI_TICK:
					return true;
				case TGUI_KEY_DOWN:
				case TGUI_KEY_UP:
					if (memcmp(&e.keyboard, &e2.keyboard, sizeof(e.keyboard)) == 0) {
						return true;
					}
					break;
				case TGUI_MOUSE_DOWN:
				case TGUI_MOUSE_UP:
				case TGUI_MOUSE_AXIS:
				case TGUI_MOUSE_WHEEL:
					if (memcmp(&e.mouse, &e2.mouse, sizeof(e.mouse)) == 0) {
						return true;
					}
					break;
				case TGUI_JOY_DOWN:
				case TGUI_JOY_UP:
				case TGUI_JOY_AXIS:
					if (memcmp(&e.joystick, &e2.joystick, sizeof(e.joystick)) == 0) {
						return true;
					}
					break;
				case TGUI_FOCUS:
					if (memcmp(&e.focus, &e2.focus, sizeof(e.focus)) == 0) {
						return true;
					}
					break;
				case TGUI_TEXT:
					if (memcpy(&e.text, &e2.text, sizeof(e.text)) == 0) {
						return true;
					}
					break;
			}
		}
	}
	
	SDL_Event events[100];
	int n;
	n = SDL_PeepEvents(&events[0], 100, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
	for (int i = 0; i < n; i++) {
		SDL_Event &e2 = events[i];
		if (e2.type == SDL_EVENT_KEY_DOWN && e.type == TGUI_KEY_DOWN) {
			if (e.keyboard.code == e2.key.keysym.sym && e.keyboard.is_repeat == (e2.key.repeat != 0)) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_KEY_UP && e.type == TGUI_KEY_UP) {
			if (e.keyboard.code == e2.key.keysym.sym && e.keyboard.is_repeat == (e2.key.repeat != 0)) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.type == TGUI_MOUSE_DOWN) {
			if (e.mouse.button == e2.button.button && e.mouse.x == e2.button.x && e.mouse.y == e2.button.y) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_MOUSE_BUTTON_UP && e.type == TGUI_MOUSE_UP) {
			if (e.mouse.button == e2.button.button && e.mouse.x == e2.button.x && e.mouse.y == e2.button.y) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_MOUSE_MOTION && e.type == TGUI_MOUSE_AXIS) {
			if (e.mouse.x == e2.motion.x && e.mouse.y == e2.motion.y) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_MOUSE_WHEEL && e.type == TGUI_MOUSE_WHEEL) {
			if (e.mouse.x == e2.wheel.x && e.mouse.y == e2.wheel.y) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && e.type == TGUI_JOY_DOWN) {
			if (e.joystick.id == e2.cbutton.which && e.joystick.button == e2.cbutton.button) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_GAMEPAD_BUTTON_UP && e.type == TGUI_JOY_UP) {
			if (e.joystick.id == e2.jbutton.which && e.joystick.button == e2.jbutton.button) {
				return true;
			}
		}
		if (e2.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && e.type == TGUI_JOY_AXIS) {
			float v = TGUI4_NORMALISE_JOY_AXIS(e2.caxis.value);
			if (e2.caxis.which == e.joystick.id && e2.caxis.axis == e.joystick.axis && v == e.joystick.value) {
				return true;
			}
		}
		if (e2.type == shim::timer_event_id && e.type == TGUI_TICK) {
			return true;
		}
	}

	return false;
}
*/

bool update()
{
	if (waiting_for_fullscreen_change) {
		return false;
	}

	if (waiting_for_resize) {
		return false;
	}

	if (quitting) {
		return false;
	}

	input::update();

	bool jic = input::is_joystick_connected();
	if (jic && joystick_was_connected == false) {
		last_mouse_event = SDL_GetTicks();
	}
	joystick_was_connected = jic; // for use with auto-hiding mouse cursor

	if (guis.size() > 0) {
		gui::GUI *noo_gui = guis[guis.size()-1];
		std::vector<gui::GUI *> other_guis;
		other_guis.insert(other_guis.begin(), guis.begin(), guis.end()-1);
		if (noo_gui->gui && noo_gui->gui->get_focus() == 0) {
			noo_gui->gui->set_focus(noo_gui->focus);
		}
		if (noo_gui->is_transitioning_out() == false) {
			noo_gui->update();
		}
		// Not else if here, the state could change in update()
		if (noo_gui->is_transitioning_out() && noo_gui->is_transition_out_finished()) {
			// update may have pushed other GUIs on the stack, so we can't just erase the last one
			for (size_t i = 0; i < guis.size(); i++) {
				if (guis[i] == noo_gui) {
					guis.erase(guis.begin() + i);
					delete noo_gui;
					break;
				}
			}
		}
		for (size_t i = 0; i < other_guis.size(); i++) {
			noo_gui = other_guis[i];
			if (noo_gui->gui && noo_gui->gui->get_focus() != 0) {
				noo_gui->focus = noo_gui->gui->get_focus();
				noo_gui->gui->set_focus(0);
			}
			if (noo_gui->is_transitioning_out() == false) {
				noo_gui->update_background();
			}
			else if (noo_gui->is_transition_out_finished()) {
				// update may have pushed other GUIs on the stack, so we can't just erase the last one
				for (size_t j = 0; j < guis.size(); j++) {
					if (guis[j] == noo_gui) {
						guis.erase(guis.begin() + j);
						delete noo_gui;
						break;
					}
				}
			}
		}
	}

	audio::lock_mutex();
	int sz = audio::internal::audio_callbacks.size();
	for (int i = 0; i < sz; i++) {
		util::Callback cb = audio::internal::audio_callbacks.back();
		void *d = audio::internal::audio_callback_data.back();
		cb(d);
		audio::internal::audio_callbacks.pop_back();
		audio::internal::audio_callback_data.pop_back();
	}
	audio::unlock_mutex();

	return true;
}

void push_event(TGUI_Event event)
{
	pushed_events.push_back(event);
}

namespace internal {

TGUI_Event *handle_tgui_event(TGUI_Event *event)
{
	*tgui_event = *event;
	return real_handle_tgui_event(tgui_event);
}

}

} // End namespace shim

} // End namespace noo
