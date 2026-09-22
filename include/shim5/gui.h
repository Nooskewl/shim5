#ifndef NOO_GUI_H
#define NOO_GUI_H

#include "shim5/main.h"
#include "shim5/shim.h"
#include "shim5/translation.h"
#include "shim5/widgets.h"

namespace noo {

namespace gui {

struct Multi_Button_GUI_Callback_Data {
	int choice;
	bool cancelled;
	void *userdata;
};

EXPORT_CLASS_ALIGN(GUI, 16) {
public:
	TGUI *gui;
	TGUI_Widget *focus; // backup focus

	GUI();
	virtual ~GUI();

	bool is_transitioning_in();
	bool is_transitioning_out();
	bool is_transition_out_finished();

	virtual void handle_event(TGUI_Event *event);

	virtual void update();
	virtual void update_background(); // called when the GUI is not the foremost

	void pre_draw(); // special stuff (starts transition timer)

	virtual void draw_back();
	virtual void draw();
	virtual void draw_fore();

	virtual void resize(util::Size<int> new_size);

	virtual bool is_fullscreen(); // if the top gui returns true, other guis don't get drawn

	bool do_return(bool ret);

	virtual bool transition_done(bool transition_in); // return true to cancel and keep this GUI alive

	virtual void transition_start(float p);
	virtual void transition_end();

	// normally a fade is done if transitions are enabled, but these can be used instead
	void use_enlarge_transition(bool onoff);
	void use_shrink_transition(bool onoff);
	void use_slide_transition(bool onoff);
	void use_slide_vertical_transition(bool onoff);

	void exit(); // call this to exit this GUI and remove it from shim::guis after transition and update()

	virtual void lost_device();
	virtual void found_device();

	virtual void transition_in_done(); // called when transition in is done (only if transition is true)

	void set_transition(bool transition);

	// For 16 byte alignment to make glm::mat4 able to use SIMD

protected:
	static const int MAX_FADE_SCALE = 10;
	
	static bool started_transition_timer;
	static Uint32 transition_start_time;

	void scale_transition(float scale);
	void slide_transition(float x);
	void slide_vertical_transition(float y);

	bool transition;
	bool transitioning_in;
	bool transitioning_out;
	bool transition_is_enlarge;
	bool transition_is_shrink;
	bool transition_is_slide;
	bool transition_is_slide_vertical;
	glm::mat4 mv_backup;
	glm::mat4 p_backup;
	float last_transition_p;
	int transition_duration;
	float slide_save;
};

class Multi_Button_GUI : public GUI
{
public:
	Multi_Button_GUI(std::string text, bool escape_cancels, std::string b1 = "OK", std::string b2 = "", std::string b3 = "", util::Callback callback = 0, void *callback_data = 0, bool shrink_to_fit = true);
	virtual ~Multi_Button_GUI();

	void update();
	void handle_event(TGUI_Event *event);

	void set_selected(int sel);

	bool get_escape_cancels();

private:
	Widget_Text_Button *wb1;
	Widget_Text_Button *wb2;
	Widget_Text_Button *wb3;

	bool escape_cancels;

	util::Callback callback;
	void *callback_data;
	
	int count;
};

// Functions
int SHIM5_EXPORT popup(std::string caption, std::string text, std::string b1 = "OK", std::string b2 = "", std::string b3 = "");
int SHIM5_EXPORT fatalerror(std::string caption, std::string text, bool do_exit = false);

} // End namespace gui

} // End namespace noo

#endif // NOO_GUI_H
