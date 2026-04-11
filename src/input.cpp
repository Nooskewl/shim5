#include "shim5/gfx.h"
#include "shim5/input.h"
#include "shim5/shim.h"
#include "shim5/util.h"

#define USE_CONSTANT_RUMBLE 0

#define REPEAT_VEC std::vector<Joy_Repeat>

using namespace noo;

#include "shim5/internal/gfx.h"
#include "shim5/internal/shim.h"

struct Joy_Repeat {
	bool is_button;
	int button;
	int axis;
	int value;
	Uint32 initial_press_time;
	bool down;
	bool repeated;
};

struct Mouse_Button_Repeat {
	bool is_touch;
	int button;
	SDL_FingerID finger;
	util::Point<float> down_pos;
	Uint32 initial_press_time;
	bool down;
};

enum Joystick_Type {
	XBOX,
	NINTENDO,
	PLAYSTATION,
	XBONE
};

struct Joystick {
	SDL_Haptic *haptic;
#if USE_CONSTANT_RUMBLE
	SDL_HapticEffect haptic_effect;
	int haptic_effect_id;
#endif
	SDL_Gamepad *gc;
	SDL_Joystick *joy;
	SDL_JoystickID id;
	Joystick_Type type;
	REPEAT_VEC joystick_repeats;
};

static std::vector<Joystick> joysticks;
static int num_joysticks;
static std::vector<Mouse_Button_Repeat> mouse_button_repeats;

static Joystick *find_joystick(SDL_JoystickID id)
{
	for (size_t i = 0; i < joysticks.size(); i++) {
		if (joysticks[i].id == id) {
			return &joysticks[i];
		}
	}

	return 0;
}

static int find_joy_repeat(bool is_button, int n, Joystick *js)
{
	REPEAT_VEC &joystick_repeats = js->joystick_repeats;

	for (size_t i = 0; i < joystick_repeats.size(); i++) {
		if (is_button && joystick_repeats[i].is_button && joystick_repeats[i].button == n) {
			return (int)i;
		}
		if (!is_button && !joystick_repeats[i].is_button && joystick_repeats[i].axis == n) {
			return (int)i;
		}
	}

	return -1;
}

static int find_mouse_button_repeat(bool is_touch, int button, SDL_FingerID finger)
{
	for (size_t i = 0; i < mouse_button_repeats.size(); i++) {
		if (mouse_button_repeats[i].is_touch == is_touch) {
			if (is_touch && mouse_button_repeats[i].finger == finger) {
				return (int)i;
			}
			else if (is_touch == false && mouse_button_repeats[i].button == button) {
				return (int)i;
			}
		}
	}

	return -1;
}

static bool check_joystick_repeat(Joy_Repeat &jr)
{
	// these define the repeat rate in thousands of a second
	int initial_delay = 500;
	int repeat_delay = 50;
	Uint32 diff = SDL_GetTicks() - jr.initial_press_time;
	if ((int)diff > initial_delay) {
		diff -= initial_delay;
		int elapsed = diff;
		int mod = elapsed % repeat_delay;
		if (mod < repeat_delay / 2) {
			// down
			if (jr.down == false) {
				jr.down = true;
				return true;
			}
		}
		else {
			// up
			jr.down = false;
		}
	}

	return false;
}

static bool check_mouse_button_repeat(Mouse_Button_Repeat &mr)
{
	// these define the repeat rate in thousands of a second
	int initial_delay = 500;
	int repeat_delay = 50;
	Uint32 diff = SDL_GetTicks() - mr.initial_press_time;
	if ((int)diff > initial_delay) {
		diff -= initial_delay;
		int elapsed = diff;
		int mod = elapsed % repeat_delay;
		if (mod < repeat_delay / 2) {
			// down
			if (mr.down == false) {
				mr.down = true;
				return true;
			}
		}
		else {
			// up
			mr.down = false;
		}
	}

	return false;
}

static void add_haptics(Joystick *j)
{
	if (SDL_IsJoystickHaptic(j->joy)) {
		util::infomsg("Joystick has haptics.\n");
		j->haptic = SDL_OpenHapticFromJoystick(j->joy);
		if (j->haptic == 0) {
			util::infomsg("Haptic init failed: %s.\n", SDL_GetError());
		}
		else {
#if USE_CONSTANT_RUMBLE
			memset(&j->haptic_effect, 0, sizeof(j->haptic_effect));
			j->haptic_effect.type = SDL_HAPTIC_CONSTANT;
			j->haptic_effect.constant.level = 0x7fff;
			j->haptic_effect.constant.length = 1000;
			j->haptic_effect_id = SDL_CreateHapticEffect(j->haptic, &j->haptic_effect);
			if (j->haptic_effect_id < 0) {
				util::infomsg("Couldn't create constant haptic effect.\n");
			}
#else
			if (SDL_InitHapticRumble(j->haptic) != 0) {
				util::infomsg("Can't init rumble effect: %s\n", SDL_GetError());
			}
#endif
			if (SDL_SetHapticGain(j->haptic, 100)) {
				util::infomsg("Can't set haptic gain: %s\n", SDL_GetError());
			}
		}
	}
	else {
		util::infomsg("Joystick does not have haptics\n");
		j->haptic = 0;
	}
}

static void check_joysticks()
{
	Joystick j;

	int nj;
	SDL_JoystickID *ids = SDL_GetGamepads(&nj);
	if (nj != num_joysticks) {
		if (nj == 0 && num_joysticks != 0) {
			if (shim::joystick_disconnect_callback) {
				shim::joystick_disconnect_callback();
			}
		}
		num_joysticks = nj;
		for (size_t i = 0; i < joysticks.size(); i++) {
			if (joysticks[i].haptic) {
				SDL_CloseHaptic(joysticks[i].haptic);
			}
			SDL_CloseGamepad(joysticks[i].gc);
		}
		joysticks.clear();
		for (int i = 0; i < num_joysticks; i++) {
			j.gc = SDL_OpenGamepad(ids[i]);
			if (j.gc == NULL) {
				util::infomsg("Error opening game controller: %s\n", SDL_GetError());

				/*
				SDL_Joystick *joy = SDL_OpenJoystick(i);
				SDL_GUID guid;
				Uint16 vendor;
				Uint16 product;
				guid = SDL_GetJoystickGUID(joy);
				SDL_GetJoystickGUIDInfo(guid, &vendor, &product, NULL);
				char *c = (char *)&guid;
				printf("guid=");
				for (int j = 0; j < 16; j++) {
					printf("%02x", c[j]);
				}
				printf("\n");
				printf("vendor=%x product=%x\n", guid, vendor, product);
				SDL_CloseJoystick(joy);
				*/

				continue;
			}
			std::string name = SDL_GetGamepadName(j.gc);
			name = util::uppercase(name);
			if (name.find("PS2") != std::string::npos || name.find("PS3") != std::string::npos || name.find("PS4") != std::string::npos || name.find("PLAYSTATION") != std::string::npos || name.find("DUALSHOCK") != std::string::npos) {
				j.type = PLAYSTATION;
			}
			else if (name.find("NINTENDO") != std::string::npos || name.find("SWITCH") != std::string::npos) {
				j.type = NINTENDO;
			}
			else if (name.find("XBOX ONE") != std::string::npos || name.find("X-BOX ONE") != std::string::npos) {
				j.type = XBONE;
			}
			else {
				j.type = XBOX;
			}
			j.joy = SDL_GetGamepadJoystick(j.gc);
			if (SDL_GetNumJoystickButtons(j.joy) < 5) {
				SDL_CloseGamepad(j.gc);
				continue;
			}
			else {
				j.id = SDL_GetJoystickID(j.joy);
				add_haptics(&j);
				joysticks.push_back(j);
			}
			// FIXME: support multiple joysticks
			//break;
		}
		//gfx::show_mouse_cursor(joysticks.size() == 0);
	}
}

namespace noo {

namespace input {

bool start()
{
	int index;
	if ((index = util::check_args(shim::argc, shim::argv, "+joystick-activate-threshold") > 0)) {
		shim::joystick_activate_threshold = atof(shim::argv[index+1]);
	}
	else {
		shim::joystick_activate_threshold = 0.8f;
	}
	if ((index = util::check_args(shim::argc, shim::argv, "+joystick-deactivate-threshold") > 0)) {
		shim::joystick_deactivate_threshold = atof(shim::argv[index+1]);
	}
	else {
		shim::joystick_deactivate_threshold = 0.75f;
	}

	joysticks.clear();
	num_joysticks = 0;

 	check_joysticks();

	return true;
}

void reset()
{
	for (size_t i = 0; i < joysticks.size(); i++) {
		Joystick &j = joysticks[i];
			if (j.haptic) {
			       SDL_CloseHaptic(j.haptic);
			}
		if (j.gc) {
			SDL_CloseGamepad(j.gc);
		}
	}

	joysticks.clear();

	num_joysticks = 0;
}

void end()
{
	reset();
}

void update()
{
	check_joysticks();

	// joystick repeat
	for (size_t j = 0; j < joysticks.size(); j++) {
		REPEAT_VEC &joystick_repeats = joysticks[j].joystick_repeats;
		for (size_t i = 0; i < joystick_repeats.size(); i++) {
			Joy_Repeat &jr = joystick_repeats[i];
			if (check_joystick_repeat(jr)) {
				jr.repeated = true;
				TGUI_Event event;
				if (jr.is_button) {
					event.type = TGUI_JOY_DOWN;
					event.joystick.is_repeat = true;
					event.joystick.button = jr.button;
					event.joystick.id = joysticks[j].id;
					shim::push_event(event);
				}
				else {
					event.type = TGUI_JOY_AXIS;
					event.joystick.is_repeat = true;
					event.joystick.axis = jr.axis;
					event.joystick.value = jr.value;
					event.joystick.id = joysticks[j].id;
					shim::push_event(event);
				}
			}
		}
	}

	// mouse button repeat
	if (shim::mouse_button_repeats) {
		for (size_t i = 0; i < mouse_button_repeats.size(); i++) {
			Mouse_Button_Repeat &mr = mouse_button_repeats[i];
			if (check_mouse_button_repeat(mr)) {
				TGUI_Event event;
				event.type = TGUI_MOUSE_DOWN;
				event.mouse.is_touch = mr.is_touch;
				event.mouse.is_repeat = true;
				event.mouse.button = mr.button;
				event.mouse.finger = mr.finger;
				event.mouse.x = mr.down_pos.x * shim::scale + shim::screen_offset.x;
				event.mouse.y = mr.down_pos.y * shim::scale + shim::screen_offset.y;
				event.mouse.dx = 0;
				event.mouse.dy = 0;
				event.mouse.normalised = false;
				shim::push_event(event);
			}
		}
	}
}

void handle_event(TGUI_Event *event)
{
	// FIXME: support multiple joysticks
	if (event->type == TGUI_JOY_DOWN || event->type == TGUI_JOY_UP || event->type == TGUI_JOY_AXIS) {
		Joystick *js = find_joystick(event->joystick.id);

		if (js == 0) {
			event->type = TGUI_UNKNOWN;
			return;
		}
	}

	// joystick button repeat
	if ((event->type == TGUI_JOY_DOWN || event->type == TGUI_JOY_UP)) {
		Joystick *js = find_joystick(event->joystick.id);

		if (js == 0) {
			return;
		}

		REPEAT_VEC &joystick_repeats = js->joystick_repeats;

		if (event->type == TGUI_JOY_DOWN) {
			if (find_joy_repeat(true, event->joystick.button, js) < 0 && event->joystick.is_repeat == false) {
				Joy_Repeat jr;
				jr.is_button = true;
				jr.button = event->joystick.button;
				jr.initial_press_time = SDL_GetTicks();
				jr.down = true;
				joystick_repeats.push_back(jr);
			}
		}
		else {
			int index = find_joy_repeat(true, event->joystick.button, js);
			if (index >= 0) {
				joystick_repeats.erase(joystick_repeats.begin() + index);
			}
		}
	}
	else if (event->type == TGUI_MOUSE_DOWN || event->type == TGUI_MOUSE_UP) {
		if (event->type == TGUI_MOUSE_DOWN) {
			if (find_mouse_button_repeat(event->mouse.is_touch, event->mouse.button, event->mouse.finger) < 0 && event->mouse.is_repeat == false) {
				Mouse_Button_Repeat mr;
				mr.is_touch = event->mouse.is_touch;
				mr.button = event->mouse.button;
				mr.finger = event->mouse.finger;
				mr.down_pos = {event->mouse.x, event->mouse.y};
				mr.initial_press_time = SDL_GetTicks();
				mr.down = true;
				mouse_button_repeats.push_back(mr);
			}
		}
		else {
			int index = find_mouse_button_repeat(event->mouse.is_touch, event->mouse.button, event->mouse.finger);
			if (index >= 0) {
				mouse_button_repeats.erase(mouse_button_repeats.begin() + index);
			}
		}
	}
	else if (event->type == TGUI_MOUSE_AXIS) {
		int index = find_mouse_button_repeat(event->mouse.is_touch, event->mouse.button, event->mouse.finger);
		if (index >= 0) {
			Mouse_Button_Repeat &mr = mouse_button_repeats[index];
			util::Point<float> diff = mr.down_pos - util::Point<float>(event->mouse.x, event->mouse.y);
			if (shim::mouse_button_repeat_max_movement >= 0 && (fabsf(diff.x) > shim::scale*shim::mouse_button_repeat_max_movement || fabsf(diff.y) > shim::scale*shim::mouse_button_repeat_max_movement)) {
				mouse_button_repeats.erase(mouse_button_repeats.begin() + index);
			}
			else {
				mr.down_pos = {event->mouse.x, event->mouse.y};
			}
		}
	}
}

bool convert_to_focus_event(TGUI_Event *event, Focus_Event *focus)
{
	int x = 0;
	int y = 0;

	if (is_joystick_connected() && event->type == TGUI_JOY_AXIS && (event->joystick.axis == 0 || event->joystick.axis == 1)) {
		Joystick *js = find_joystick(event->joystick.id);

		if (js == 0) {
			return false;
		}
		
		REPEAT_VEC &joystick_repeats = js->joystick_repeats;

		int axis = event->joystick.axis;
		int index = find_joy_repeat(false, axis, js);
		if (fabsf(event->joystick.value) > shim::joystick_activate_threshold) {
			bool go = true;
			if (index < 0) {
				if (event->joystick.is_repeat == false) {
					Joy_Repeat jr;
					jr.is_button = false;
					jr.axis = axis;
					jr.value = event->joystick.value < 0 ? -1 : 1;
					jr.initial_press_time = SDL_GetTicks();
					jr.down = true;
					jr.repeated = false;
					joystick_repeats.push_back(jr);
					go = true;
				}
			}
			else {
				Joy_Repeat &jr = joystick_repeats[index];
				if (jr.repeated) {
					jr.repeated = false;
					go = true;
				}
				else {
					go = false;
				}
			}
			if (go) {
				if (axis == 0) {
					if (event->joystick.value < 0) {
						x = -1;
					}
					else {
						x = 1;
					}
				}
				else if (axis == 1) {
					if (event->joystick.value < 0) {
						y = -1;
					}
					else {
						y = 1;
					}
				}
			}
		}
		else if (index >= 0 && fabsf(event->joystick.value) < shim::joystick_deactivate_threshold) {
			joystick_repeats.erase(joystick_repeats.begin() + index);
		}
	}

	if (event->type == TGUI_KEY_DOWN) {
		if (event->keyboard.code == shim::key_l) {
			x = -1;
		}
		else if (event->keyboard.code == shim::key_r) {
			x = 1;
		}
		else if (event->keyboard.code == shim::key_u) {
			y = -1;
		}
		else if (event->keyboard.code == shim::key_d) {
			y = 1;
		}
	}

	if (event->type == TGUI_JOY_DOWN) {
		if (event->joystick.button == TGUI_B_L) {
			x = -1;
		}
		else if (event->joystick.button == TGUI_B_R) {
			x = 1;
		}
		else if (event->joystick.button == TGUI_B_U) {
			y = -1;
		}
		else if (event->joystick.button == TGUI_B_D) {
			y = 1;
		}
	}

	if (x != 0 || y != 0) {
		focus->orig_type = event->type;
		if (event->type == TGUI_KEY_DOWN) {
			focus->u.orig_keyboard.code = event->keyboard.code;
			focus->u.orig_keyboard.is_repeat = event->keyboard.is_repeat;
		}
		else {
			focus->u.orig_joystick.id = event->joystick.id;
			focus->u.orig_joystick.button = event->joystick.button;
			focus->u.orig_joystick.axis = event->joystick.axis;
			focus->u.orig_joystick.value = event->joystick.value;
			focus->u.orig_joystick.is_repeat = event->joystick.is_repeat;
		}
		focus->type = TGUI_FOCUS;
		if  (x < 0) {
			focus->focus.type = TGUI_FOCUS_LEFT;
		}
		else if (x > 0) {
			focus->focus.type = TGUI_FOCUS_RIGHT;
		}
		else if (y < 0) {
			focus->focus.type = TGUI_FOCUS_UP;
		}
		else {
			focus->focus.type = TGUI_FOCUS_DOWN;
		}
		return true;
	}
	else {
		return false;
	}
}

void convert_focus_to_original(TGUI_Event *event)
{
	if (event->type == TGUI_FOCUS) {
		// grab the original...
		input::Focus_Event *focus = dynamic_cast<input::Focus_Event *>(event);
		if (focus) {
			event->type = focus->orig_type;
			if (focus->orig_type == TGUI_KEY_DOWN) {
				event->keyboard.code = focus->u.orig_keyboard.code;
				event->keyboard.is_repeat = focus->u.orig_keyboard.is_repeat;
			}
			else {
				event->joystick.id = focus->u.orig_joystick.id;
				event->joystick.button = focus->u.orig_joystick.button;
				event->joystick.axis = focus->u.orig_joystick.axis;
				event->joystick.value = focus->u.orig_joystick.value;
				event->joystick.is_repeat = focus->u.orig_joystick.is_repeat;
			}
		}
	}
}

void rumble(Uint32 length, int num)
{
	for (size_t i = 0; i < joysticks.size(); i++) {

		if (num >= 0 && (int)i != num) {
			continue;
		}

		Joystick &j = joysticks[i];

		if (j.gc) {
			SDL_RumbleGamepad(j.gc, 0x7777, 0x7777, length);
		}
	}
}

bool is_joystick_connected()
{
	return joysticks.size() != 0;
}

std::string get_joystick_button_colour_code(int button)
{
	Joystick_Type type;

	if (joysticks.size() > 0) {
		type = joysticks[0].type;
	}
	else {
		type = XBOX;
	}

	if (type == PLAYSTATION) {
		switch (button) {
			case TGUI_B_A:
				return "#7cb2e8";
			case TGUI_B_B:
				return "#ff6666";
			case TGUI_B_X:
				return "#ff69f8";
			case TGUI_B_Y:
				return "#40e2a0";
			default:
				return "";
		}
	}
	else if (type == NINTENDO) {
		return "";
	}
	else {
		switch (button) {
			case TGUI_B_A:
				return "#32c832";
			case TGUI_B_B:
				return "#c83232";
			case TGUI_B_X:
				return "#3296c8";
			case TGUI_B_Y:
				return "#c8c832";
			default:
				return "";
		}
	}

	return "";
}

std::string get_joystick_button_name(int button)
{
	Joystick_Type type;

	if (joysticks.size() > 0) {
		type = joysticks[0].type;
	}
	else {
		type = XBOX;
	}

	if (type == PLAYSTATION) {
		switch (button) {
			case TGUI_B_A:
				return "@F4";
			case TGUI_B_B:
				return "@F5";
			case TGUI_B_X:
				return "@F6";
			case TGUI_B_Y:
				return "@F7";
			case TGUI_B_LB:
				return "LB";
			case TGUI_B_RB:
				return "RB";
			case TGUI_B_BACK:
				return "SHARE";
			case TGUI_B_START:
				return "OPTIONS";
			case TGUI_B_GUIDE:
				return "PS";
			case TGUI_B_LS:
				return "LS";
			case TGUI_B_RS:
				return "RS";
			default:
				return std::string("BUTTON ") + util::itos(button);
		}
	}
	else if (type == NINTENDO) {
		switch (button) {
			case TGUI_B_A:
				return "A";
			case TGUI_B_B:
				return "B";
			case TGUI_B_X:
				return "X";
			case TGUI_B_Y:
				return "Y";
			case TGUI_B_LB:
				return "LB";
			case TGUI_B_RB:
				return "RB";
			case TGUI_B_BACK:
				return "-";
			case TGUI_B_START:
				return "+";
			case TGUI_B_GUIDE:
				return "HOME";
			case TGUI_B_LS:
				return "LS";
			case TGUI_B_RS:
				return "RS";
			default:
				return std::string("BUTTON ") + util::itos(button);
		}
	}
	else {
		switch (button) {
			case TGUI_B_A:
				return "@F0";
			case TGUI_B_B:
				return "@F1";
			case TGUI_B_X:
				return "@F2";
			case TGUI_B_Y:
				return "@F3";
			case TGUI_B_LB:
				return "LB";
			case TGUI_B_RB:
				return "RB";
			case TGUI_B_BACK:
				if (type == XBONE) {
					return "VIEW";
				}
				else {
					return "BACK";
				}
			case TGUI_B_START:
				if (type == XBONE) {
					return "MENU";
				}
				else {
					return "START";
				}
			case TGUI_B_GUIDE:
				if (type == XBONE) {
					return "XBOX";
				}
				else {
					return "GUIDE";
				}
			case TGUI_B_LS:
				return "LS";
			case TGUI_B_RS:
				return "RS";
			default:
				return std::string("BUTTON ") + util::itos(button);
		}
	}
}

void drop_repeats(bool joystick, bool mouse)
{
	if (joystick) {
		for (size_t i = 0; i < joysticks.size(); i++) {
			joysticks[i].joystick_repeats.clear();
		}
	}
	if (mouse) {
		mouse_button_repeats.clear();
	}
}

int get_num_joysticks()
{
	return joysticks.size();
}

int get_controller_index(SDL_JoystickID id)
{
	for (size_t i = 0; i < joysticks.size(); i++) {
		if (joysticks[i].id == id) {
			return i;
		}
	}
	return -1;
}

SDL_JoystickID get_controller_id(int index)
{
	if ((int)joysticks.size() <= index) {
		return 0;
	}
	return joysticks[index].id;
}

SDL_Joystick *get_sdl_joystick(SDL_JoystickID id)
{
	Joystick *j = find_joystick(id);
	if (j) {
		return j->joy;
	}
	else {
		return nullptr;
	}
}

SDL_Gamepad *get_sdl_gamepad(SDL_JoystickID id)
{
	Joystick *j = find_joystick(id);
	if (j) {
		return j->gc;
	}
	else {
		return nullptr;
	}
}

Focus_Event::~Focus_Event()
{
}

bool system_has_touchscreen()
{
	static bool cached = false;
	static bool result;

	if (cached) {
		return result;
	}

	result = GetSystemMetrics(/*SM_MAXIMUMTOUCHES*/95) > 0; // SM_MAXIMUMTOUCHES is only available on Windows 7+
	
	cached = true;

	return result;
}

bool system_has_keyboard()
{
	// Kind of a hack. If it's a tablet we want non-keyboard behaviour...
	if (shim::force_tablet) {
		return GetSystemMetrics(SM_TABLETPC) == 0;
	}
	else {
		return true;
	}
}


} // End namespace input

} // End namespace noo
