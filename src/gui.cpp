#include "shim5/font.h"
#include "shim5/gfx.h"
#include "shim5/gui.h"
#include "shim5/image.h"
#include "shim5/mml.h"
#include "shim5/shader.h"
#include "shim5/shim.h"
#include "shim5/sprite.h"
#include "shim5/translation.h"
#include "shim5/util.h"
#include "shim5/util.h"

#include "shim5/internal/gfx.h"
#include "shim5/internal/shim.h"

using namespace noo;

namespace noo {

namespace gui {

bool GUI::started_transition_timer = false;
Uint32 GUI::transition_start_time = 0;

GUI::GUI() :
	gui(0),
	focus(0),
	transition_duration(250),
	slide_save(0.0f)
{
	transition = false; // set this true to do transitions
	transitioning_in = true;
	transitioning_out = false;
	transition_is_enlarge = false;
	transition_is_shrink = false;
	transition_is_slide = false;
	transition_is_slide_vertical = false;
}

GUI::~GUI()
{
	delete gui;
	if (shim::guis.size() == 0) {
		started_transition_timer = false;
	}
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
		else {
			fade_transition(p);
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
		else {
			p = 1.0f - p;
			fade_transition(p);
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
	else {
		gfx::set_target_backbuffer();
		Uint8 c = Uint8(last_transition_p * 255);
		SDL_Color whitish = { c, c, c, c };
		glm::mat4 mv_backup, proj_backup, mv;
		gfx::get_matrices(mv_backup, proj_backup);
		gfx::set_default_projection(shim::screen_size, shim::screen_offset, shim::scale);
		gfx::update_projection();
		gfx::internal::gfx_context.work_image->draw_tinted(whitish, util::Point<int>(0, 0));
		gfx::set_matrices(mv_backup, proj_backup);
		gfx::update_projection();
	}
}

void GUI::fade_transition(float p)
{
	last_transition_p = p;
	gfx::set_target_image(gfx::internal::gfx_context.work_image);
	gfx::clear(shim::transparent);
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
	if (started_transition_timer == false) {
		transition_start_time = SDL_GetTicks();
		started_transition_timer = true;
	}
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
}

void GUI::update_background()
{
}

bool GUI::transition_done(bool transition_in)
{
	return false;
}

//--

int popup(std::string caption, std::string text, Popup_Type type)
{
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
	int ret = MessageBox(gfx::internal::gfx_context.hwnd, text.c_str(), caption.c_str(), native_type);
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

int fatalerror(std::string caption, std::string text, Popup_Type type, bool do_exit)
{
	delete_shim_args();
	shim::argc = 3;
	shim::argv = new char *[shim::argc];
	shim::argv[0] = new char[2];
	strcpy(shim::argv[0], "x");
	shim::argv[1] = new char[10];
	strcpy(shim::argv[1], "+windowed");
	shim::argv[2] = new char[8];
	strcpy(shim::argv[2], "+opengl");

	try {
		gfx::restart(1280, 720, false, 1280, 720);
	}
	catch (util::Error &e) {
		// do nothing
	}
	
	SDL_Delay(250);

	int ret = popup(caption, text, type);
	if (do_exit) {
		exit(1);
	}
	return ret;
}
	
} // End namespace gui

} // End namespace noo
