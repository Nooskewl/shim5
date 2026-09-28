#include "shim5/shim.h"
#include "shim5/font.h"
#include "shim5/gfx.h"
#include "shim5/gui.h"
#include "shim5/image.h"
#include "shim5/mml.h"
#include "shim5/shader.h"
#include "shim5/sprite.h"
#include "shim5/translation.h"
#include "shim5/util.h"
#include "shim5/util.h"

#include "shim5/internal/gfx.h"
#include "shim5/internal/shim.h"

using namespace noo;

const int WIN_BORDER = 10;

namespace noo {

namespace gui {

GUI::GUI() :
	gui(0),
	focus(0),
	transition_duration(250),
	slide_save(0.0f),
	done_transition_in(false)
{
	transition = shim::gui_transition_in_type != TRANSITION_NONE;
	transitioning_in = true;
	transitioning_out = false;
	transition_is_enlarge = false;
	transition_is_shrink = false;
	transition_is_slide = false;
	transition_is_slide_vertical = false;
	transition_start_time = SDL_GetTicks();

	switch (shim::gui_transition_in_type) {
		case TRANSITION_ENLARGE:
			transition_is_enlarge = true;
			break;
		case TRANSITION_SHRINK:
			transition_is_shrink = true;
			break;
		case TRANSITION_SLIDE:
			transition_is_slide = true;
			break;
		case TRANSITION_SLIDE_VERTICAL:
			transition_is_slide_vertical = true;
			break;

	}
}

GUI::~GUI()
{
	delete gui;
}

void GUI::handle_event(TGUI_Event *event) {
	if (gui) {
		TGUI_Widget *focus = gui->get_focus();
		gui->handle_event(event);
		if (event->type == TGUI_FOCUS && focus != gui->get_focus()) {
			if (shim::widget_sfx != nullptr) {
				shim::widget_sfx->play();
			}
		}
	}
}

void GUI::draw_back()
{
	if (transition == false) {
		transitioning_in = false; // for guis that don't transition in (otherwise out looks like in)
		return;
	}

	float p = (SDL_GetTicks() - transition_start_time) / (float)transition_duration;
	if (p >= 1.0f) {
		p = 1.0f;
	}

	if (transitioning_in) {
		if (p >= 1.0f) {
			p = 1.0f;
			transitioning_in = false;
			transition_done(true);
			transition_in_done();
		}
		else {
			transition_start(p);
		}
	}
	else if (transitioning_out) {
		transition_start(p);
	}
}

void GUI::draw()
{
	if (gui) {
		gui->draw();
	}
}

void GUI::draw_fore()
{
	if (transition == false) {
		return;
	}

	if (transitioning_out || transitioning_in) {
		transition_end();
	}
}

void GUI::resize(util::Size<int> size)
{
	gui->resize(size.w, size.h);
}

bool GUI::is_fullscreen()
{
	return false;
}

bool GUI::is_transition_out_finished() {
	if (transitioning_out) {
		if (transition == false) {
			return true;
		}
		else {
			if (int(SDL_GetTicks() - transition_start_time) >= transition_duration) {
				if (transition_done(false) == false) {
					return true;
				}
				else {
					transitioning_out = false;
				}
			}
		}
	}

	return false;
}

void GUI::exit()
{
	transitioning_out = true;
	transition_start_time = SDL_GetTicks();
}

void GUI::transition_start(float p)
{
	if (transitioning_in) {
		if (transition_is_enlarge || transition_is_shrink) {
			float scale;
			if (transition_is_enlarge) {
				scale = 1.0f + (1.0f - p) * (MAX_FADE_SCALE-1);
			}
			else {
				scale = p;
			}
			scale_transition(scale);
		}
		else if (transition_is_slide) {
			slide_transition(p-1.0f);
		}
		else if (transition_is_slide_vertical) {
			slide_vertical_transition(p-1.0f);
		}
	}
	else {
		if (transition_is_enlarge || transition_is_shrink) {
			float scale;
			if (transition_is_enlarge) {
				scale = 1.0f + p * (MAX_FADE_SCALE-1);
			}
			else {
				scale = 1.0f - p;
			}
			scale_transition(scale);
		}
		else if (transition_is_slide) {
			slide_transition(p);
		}
		else if (transition_is_slide_vertical) {
			slide_vertical_transition(p);
		}
	}
}

void GUI::transition_end()
{
	if (transition_is_enlarge || transition_is_shrink) {
		glm::mat4 mv, p;
		gfx::get_matrices(mv, p);
		gfx::set_matrices(mv_backup, p);
		gfx::update_projection();
	}
	else if (transition_is_slide || transition_is_slide_vertical) {
		gfx::set_default_projection(shim::screen_size, shim::screen_offset, shim::scale);
		gfx::update_projection();
	}
}

void GUI::scale_transition(float scale)
{
	//scale *= shim::scale;
	int new_w = int(shim::screen_size.w * scale);
	int new_h = int(shim::screen_size.h * scale);
	//int w_diff = (new_w - shim::real_screen_size.w) / 2;
	//int h_diff = (new_h - shim::real_screen_size.h) / 2;
	int w_diff = (new_w - shim::screen_size.w) / 2;
	int h_diff = (new_h - shim::screen_size.h) / 2;
	glm::mat4 mv, p;
	gfx::get_matrices(mv_backup, p);
	mv = glm::mat4();
	mv = glm::translate(mv, glm::vec3(-w_diff, -h_diff, 0.0f));
	mv = glm::scale(mv, glm::vec3(scale, scale, 1.0f));
	gfx::set_matrices(mv, p);
	gfx::update_projection();
}

void GUI::slide_transition(float p)
{
	slide_save = p;
	gfx::set_default_projection(shim::screen_size, shim::screen_offset, shim::scale);
	glm::mat4 mv, proj;
	gfx::get_matrices(mv, proj);
	mv = glm::translate(mv, glm::vec3(p * shim::screen_size.w, 0.0f, 0.0f));
	gfx::set_matrices(mv, proj);
	gfx::update_projection();
}

void GUI::slide_vertical_transition(float p)
{
	slide_save = p;
	gfx::set_default_projection(shim::screen_size, shim::screen_offset, shim::scale);
	glm::mat4 mv, proj;
	gfx::get_matrices(mv, proj);
	mv = glm::translate(mv, glm::vec3(0.0f, p * shim::screen_size.h, 0.0f));
	gfx::set_matrices(mv, proj);
	gfx::update_projection();
}

void GUI::use_enlarge_transition(bool onoff)
{
	transition_is_enlarge = onoff;
}

void GUI::use_shrink_transition(bool onoff)
{
	transition_is_shrink = onoff;
}

void GUI::use_slide_transition(bool onoff)
{
	transition_is_slide = onoff;
}

void GUI::use_slide_vertical_transition(bool onoff)
{
	transition_is_slide_vertical = onoff;
}

void GUI::lost_device()
{
}

void GUI::found_device()
{
}

void GUI::transition_in_done()
{
	slide_save = 0.0f;
}

void GUI::set_transition(bool transition)
{
	this->transition = transition;
}

void GUI::pre_draw()
{
}

bool GUI::is_transitioning_in()
{
	return transitioning_in;
}

bool GUI::is_transitioning_out()
{
	return transitioning_out;
}

void GUI::update()
{
	if (transitioning_in == false && done_transition_in == false) {
		switch (shim::gui_transition_in_type) {
			case TRANSITION_ENLARGE:
				transition_is_enlarge = false;
				break;
			case TRANSITION_SHRINK:
				transition_is_shrink = false;
				break;
			case TRANSITION_SLIDE:
				transition_is_slide = false;
				break;
			case TRANSITION_SLIDE_VERTICAL:
				transition_is_slide_vertical = false;
				break;

		}
		switch (shim::gui_transition_out_type) {
			case TRANSITION_ENLARGE:
				transition_is_enlarge = true;
				break;
			case TRANSITION_SHRINK:
				transition_is_shrink = true;
				break;
			case TRANSITION_SLIDE:
				transition_is_slide = true;
				break;
			case TRANSITION_SLIDE_VERTICAL:
				transition_is_slide_vertical = true;
				break;

		}
		transition = shim::gui_transition_out_type != TRANSITION_NONE;
		done_transition_in = true;
	}
}

void GUI::update_background()
{
}

bool GUI::transition_done(bool transition_in)
{
	return false;
}

//--

Multi_Button_GUI::Multi_Button_GUI(std::string text, bool escape_cancels, std::string b1, std::string b2, std::string b3, util::Callback callback, void *callback_data, bool shrink_to_fit) :
	escape_cancels(escape_cancels),
	callback(callback),
	callback_data(callback_data),
	count(0)
{
	Widget *modal_main_widget = new Widget(1.0f, 1.0f);

	int window_w = int(shim::screen_size.w * 0.75f);

	bool full;
	int num_lines, width;
	int line_height = shim::font->get_height() + 1;
	shim::font->draw_wrapped(shim::white, text, util::Point<int>(0, 0), window_w - WIN_BORDER*4, line_height, -1, -1, 0, true, full, num_lines, width);
	
	wb1 = new Widget_Text_Button(b1);
	if (b2 == "") {
		wb2 = nullptr;
	}
	else {
		wb2 = new Widget_Text_Button(b2);
	}
	if (b3 == "") {
		wb3 = nullptr;
	}
	else {
		wb3 = new Widget_Text_Button(b3);
	}

	if (shrink_to_fit) {
		if (wb3) {
			window_w = MIN(window_w, MAX(wb1->get_width() + wb2->get_width() + wb3->get_width() + 2, width) + WIN_BORDER * 4);
		}
		else if (wb2) {
			window_w = MIN(window_w, MAX(wb1->get_width() + wb2->get_width() + 2, width) + WIN_BORDER * 4);
		}
		else {
			window_w = MIN(window_w, MAX(wb1->get_width(), width) + WIN_BORDER * 4);
		}
	}

	Widget_Label *label = new Widget_Label(text, window_w - WIN_BORDER*4, shim::font);
	label->set_centre_x(true);
	label->set_padding(WIN_BORDER);

	wb1->set_centre_x(true);
	wb1->set_padding_right(2);

	if (wb2) {
		wb2->set_centre_x(true);
		wb2->set_padding_left(20);
	}

	if (wb3) {
		wb3->set_centre_x(true);
		wb3->set_padding_left(20);
	}

	Widget *button_container = new Widget(1.0f, wb1->get_height());
	button_container->set_float_bottom(true);

	Widget_Window *window = new Widget_Window(window_w, line_height * num_lines + wb1->get_height() + WIN_BORDER*4);
	window->set_centre_x(true);
	window->set_centre_y(true);
	window->set_parent(modal_main_widget);

	TGUI_Widget *pad = new TGUI_Widget(1.0f, 1.0f);
	pad->set_padding(WIN_BORDER);
	pad->set_parent(window);

	label->set_parent(pad);
	button_container->set_parent(pad);
	wb1->set_parent(button_container);
	if (wb2) {
		wb2->set_parent(button_container);
	}
	if (wb3) {
		wb3->set_parent(button_container);
	}

	gui = new TGUI(modal_main_widget, shim::screen_size.w, shim::screen_size.h);

	gui->set_focus(wb1);
}

Multi_Button_GUI::~Multi_Button_GUI()
{
}

void Multi_Button_GUI::update()
{
	GUI::update();

	if (transitioning_in || transitioning_out) {
		return;
	}

	if (wb1->pressed()) {
		if (callback) {
			Multi_Button_GUI_Callback_Data d;
			d.choice = 0;
			d.cancelled = false;
			d.userdata = callback_data;
			callback(&d);
		}
		exit();
	}
	else if (wb2 && wb2->pressed()) {
		if (callback) {
			Multi_Button_GUI_Callback_Data d;
			d.choice = 1;
			d.cancelled = false;
			d.userdata = callback_data;
			callback(&d);
		}
		exit();
	}
	else if (wb3 && wb3->pressed()) {
		if (callback) {
			Multi_Button_GUI_Callback_Data d;
			d.choice = 2;
			d.cancelled = false;
			d.userdata = callback_data;
			callback(&d);
		}
		exit();
	}
}

void Multi_Button_GUI::handle_event(TGUI_Event *event)
{
	if (transitioning_in || transitioning_out) {
		return;
	}

	if (escape_cancels && (
		(event->type == TGUI_KEY_DOWN && event->keyboard.code == TGUIK_ESCAPE) ||
		(event->type == TGUI_JOY_DOWN && event->joystick.button == TGUI_B_BACK)
	)) {
		if (callback) {
			Multi_Button_GUI_Callback_Data d;
			d.choice = -1;
			d.cancelled = true;
			d.userdata = callback_data;
			callback(&d);
		}
		exit();
	}
	else {
		GUI::handle_event(event);
	}
}

void Multi_Button_GUI::set_selected(int v)
{
	if (v == 0) {
		gui->set_focus(wb1);
	}
	else if (v == 1) {
		gui->set_focus(wb2);
	}
	else {
		gui->set_focus(wb3);
	}
}

bool Multi_Button_GUI::get_escape_cancels()
{
	return escape_cancels;
}

//--

static void gui_loop(gui::GUI *gui, gfx::Image *img)
{
	// These keep the logic running at 60Hz and drawing at refresh rate is possible
	// NOTE: screen refresh rate has to be 60Hz or higher for this to work.
	Uint32 start = SDL_GetTicks();
	int logic_frames = 0;
	int drawing_frames = 0;
	bool can_draw = true;
	bool can_logic = true;
	int curr_logic_rate = shim::logic_rate;

	bool quit = false;

	while (quit == false) {
		if (shim::guis.back() != gui) {
			break;
		}
		// EVENTS
		while (true) {
			SDL_Event sdl_event;
			TGUI_Event *e = nullptr;

			bool all_done = false;

			if (!SDL_PollEvent(&sdl_event)) {
				e = shim::pop_pushed_event();
				if (e == nullptr) {
					all_done = true;
				}
			}

			if (all_done) {
				break;
			}

			if (e == nullptr) {
				if (sdl_event.type == SDL_EVENT_QUIT) {
					if (can_logic == false) {
						shim::handle_event(&sdl_event);
						quit = true;
						break;
					}
				}
			}

			TGUI_Event *event;

			if (e) {
				event = e;
			}
			else {
				if (sdl_event.type == SDL_EVENT_QUIT) {
					static TGUI_Event quit_event;
					quit_event.type = TGUI_QUIT;
					event = &quit_event;
				}
				else {
					event = shim::handle_event(&sdl_event);
				}
			}

			//handle_event(event);
			if (quit) {
				break;
			}
		}

		if (quit) {
			break;
		}

		// Logic rate can change in devsettings
		if (shim::logic_rate != curr_logic_rate) {
			curr_logic_rate = shim::logic_rate;
			logic_frames = 0;
			drawing_frames = 0;
			start = SDL_GetTicks();
		}

		// TIMING
		float ms_per_logic_frame = 1000 / shim::logic_rate;
		Uint32 now = SDL_GetTicks();
		int diff = now - start;
		bool skip_drawing = false;
		int logic_reps = diff / ms_per_logic_frame;

		if (logic_reps > 0) {
			start += ms_per_logic_frame * logic_reps;
		}

		for (int logic = 0; logic < logic_reps; logic++) {
			can_draw = shim::update();

			// Generate a timer tick event (TGUI_TICK)
			SDL_Event sdl_event;
			sdl_event.type = shim::timer_event_id;
			TGUI_Event *event = shim::handle_event(&sdl_event);
			//handle_event(event);
			if (quit) {
				break;
			}

			//call_timer_callbacks(prg);

			//std::vector<Token> tmp;
			//call_function(prg, "run", tmp);

			//mouse_wheel_y = 0;

			logic_frames++;
		}

		if (quit) {
			break;
		}

		// LOGIC
		if (can_logic) {
			for (int logic = 0; logic < logic_reps; logic++) {
				gfx::update_animations();
				// logic
			}
		}

		// DRAWING
		if (skip_drawing == false && can_draw) {
			glDisable_ptr(GL_SCISSOR_TEST);
			PRINT_GL_ERROR("glDisable(GL_SCISSOR_TEST)\n");
			gfx::clear_buffers();
			glEnable_ptr(GL_SCISSOR_TEST);
			PRINT_GL_ERROR("glEnable(GL_SCISSOR_TEST)\n");

			SDL_Color tint;
			tint.r = 96;
			tint.g = 96;
			tint.b = 96;
			tint.a = 96;
			img->draw_tinted(tint, util::Point<int>(0, 0), gfx::Image::FLIP_V);

			gfx::draw_guis();
			gfx::draw_notifications();

			gfx::flip();
		}

		drawing_frames++;
	}
}

static int multi_popup_result;

static void yes_no_cb(void *data)
{
	Multi_Button_GUI_Callback_Data *d = (Multi_Button_GUI_Callback_Data *)data;
	multi_popup_result = d->choice;
}

int popup(std::string caption, std::string text, std::string b1, std::string b2, std::string b3)
{
	gfx::screen_shake(0, 0);

	glm::mat4 _mv, _proj;
	gfx::get_matrices(_mv, _proj);
	gfx::Shader *old_shader = shim::current_shader;
	shim::current_shader = shim::default_shader;
	shim::current_shader->use();
	util::Size<int> bak = shim::screen_size;
	int w2 = shim::real_screen_size.w;
	int h2 = shim::real_screen_size.h;
	double aspect = (double)w2/h2;
	gfx::set_min_aspect_ratio(aspect-0.001f);
	gfx::set_max_aspect_ratio(aspect+0.001f);
	gfx::set_scaled_size({w2, h2});
	gfx::update_projection();

	int w, h;
	unsigned char *bytes = gfx::Image::read_backbuffer(true, &w, &h);
	gfx::Image *img = new gfx::Image((Uint8 *)bytes, util::Size<int>(w, h), true);

	std::vector<GUI *> guis = shim::guis;
	shim::guis.clear();
	GUI *gui;
	caption = "#FFD800" + caption;
	text = "#FFFFFF" + text;
	gui = new Multi_Button_GUI(caption+" "+text, true, b1, b2, b3, yes_no_cb, 0, true);
	shim::guis.push_back(gui);

	shim::convert_directions_to_focus_events = true;
	int fsk = shim::fullscreen_key;
	shim::fullscreen_key = -1;
	SDL_SetWindowResizable(gfx::internal::gfx_context.window, false);
	bool rel = SDL_GetWindowRelativeMouseMode(gfx::internal::gfx_context.window);
	SDL_SetWindowRelativeMouseMode(gfx::internal::gfx_context.window, false);
	bool test = gfx::is_depth_test_enabled();
	bool write = gfx::is_depth_write_enabled();
	gfx::enable_depth_test(false);
	gfx::enable_depth_write(false);

	gui_loop(gui, img);

	shim::guis = guis;
			
	gfx::enable_depth_test(test);
	gfx::enable_depth_write(write);
	shim::current_shader = old_shader;
	shim::current_shader->use();
	w2 = bak.w;
	h2 = bak.h;
	aspect = (double)w2/h2;
	gfx::set_min_aspect_ratio(aspect-0.001f);
	gfx::set_max_aspect_ratio(aspect+0.001f);
	gfx::set_scaled_size(bak);
	gfx::set_matrices(_mv, _proj);
	gfx::update_projection();
	SDL_SetWindowRelativeMouseMode(gfx::internal::gfx_context.window, rel);
	if (shim::guis.size() == 0) {
		shim::convert_directions_to_focus_events = false;
	}
	shim::fullscreen_key = fsk;
	SDL_SetWindowResizable(gfx::internal::gfx_context.window, true);

	delete img;
	return multi_popup_result;
/*
	UINT native_type;
	if (type == OK) {
		native_type = MB_OK;
	}
	else if (type == YESNO) {
		native_type = MB_YESNO;
	}
	else {
		return -1;
	}
	int ret = MessageBoxA(gfx::internal::gfx_context.hwnd, text.c_str(), caption.c_str(), native_type);
	int result;
	if (type == OK) {
		result = 0;
	}
	else if (type == YESNO) {
		if (ret == IDYES) {
			result = 1;
		}
		else {
			result = 0;
		}
	}
	else {
		result = -1;
	}
	return result;
	*/
}

static void delete_shim_args()
{
       for (int i = 0; i < shim::argc; i++) {
               delete[] shim::argv[i];
       }
       delete[] shim::argv;
       shim::argc = 0;
       shim::argv = NULL;
}

int fatalerror(std::string caption, std::string text, bool do_exit)
{
	delete_shim_args();
	shim::argc = 3;
	shim::argv = new char *[shim::argc];
	shim::argv[0] = new char[2];
	strcpy_s(shim::argv[0], 2, "x");
	shim::argv[1] = new char[10];
	strcpy_s(shim::argv[1], 10, "+windowed");
	shim::argv[2] = new char[8];
	strcpy_s(shim::argv[2], 8, "+opengl");

	try {
		gfx::restart(1280, 720, false, 1280, 720);
	}
	catch (util::Error &e) {
		// do nothing
	}
	
	SDL_Delay(250);

	int ret = popup(caption, text);
	if (do_exit) {
		exit(1);
	}
	return ret;
}
	
} // End namespace gui

} // End namespace noo
