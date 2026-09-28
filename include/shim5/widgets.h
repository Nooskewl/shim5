#ifndef NOO_WIDGETS_H
#define NOO_WIDGETS_H

#include "shim5/main.h"
#include "shim5/font.h"
#include "shim5/json.h"

namespace noo {

namespace gui {

class SHIM5_EXPORT Widget : public TGUI_Widget
{
public:
	Widget(int w, int h);
	Widget(float percent_w, float percent_h);
	Widget(int w, float percent_h);
	Widget(float percent_w, int h);
	Widget(TGUI_Widget::Fit fit, int other);
	Widget(TGUI_Widget::Fit fit, float percent_other);
	Widget(); // Fit both
	virtual ~Widget();

	void draw();

	SDL_Color bg_colour;

protected:
	void start();
	bool is_focussed();
};

class SHIM5_EXPORT DevSettings_List : public Widget
{
public:
	DevSettings_List();
	virtual ~DevSettings_List();

	void draw();
	void handle_event(TGUI_Event *event);

	bool is_editing();

	void set_value(std::string text);

	bool is_done();

private:
	void set_nr();
	void draw_text_clamped(SDL_Color colour, std::string text, util::Point<int> pos, int max_width);
	void draw_text_scroll(SDL_Color colour, std::string text, util::Point<int> pos, int max_width);
	void save_edit();
	void draw_edit(std::string value, util::Point<int> pos, int max_width);
	void insert_text(char *text);
	void set_edit_offset();

	int top;
	int selected;
	bool editing;
	util::JSON::Node *node;
	std::string tmpval; // during editing
	int nr;
	int row_h;
	Uint32 change_time;
	int edit_offset;
	int cursor_pos;
	int val_width;

	bool done;
};

class SHIM5_EXPORT DevSettings_Label : public Widget
{
public:
	DevSettings_Label(std::string text);
	virtual ~DevSettings_Label();

	void draw();

	void set_text(std::string text);
	std::string get_text();

private:
	std::string text;
};

class Widget_Button : public Widget {
public:
	Widget_Button(int w, int h);
	Widget_Button(float w, float h);
	Widget_Button(int w, float h);
	Widget_Button(float w, int h);
	virtual ~Widget_Button();

	virtual void handle_event(TGUI_Event *event);

	virtual bool pressed();

	void set_sound_enabled(bool enabled);

	void set_pressed(bool pressed);

	void set_mouse_only(bool mouse_only);

protected:
	void start();

	bool _pressed;
	bool _released;
	bool _hover;
	bool gotten;
	bool sound_enabled;
	bool mouse_only;
};

class SHIM5_EXPORT Widget_Text_Button : public Widget_Button
{
public:
	Widget_Text_Button(std::string text);
	virtual ~Widget_Text_Button();

	void draw();

	void set_enabled(bool enabled);
	bool is_enabled();
	void set_text(std::string text);

protected:
	void set_size();

	std::string text;

	bool enabled;
};

class SHIM5_EXPORT Widget_Label : public Widget
{
public:
	Widget_Label(std::string text, int max_w, gfx::Font *font = NULL);
	virtual ~Widget_Label();

	void draw();

	void set_text(std::string text);
	void set_max_width(int width);

	std::string get_text();

private:
	void start();

	std::string text;
	int max_w;
	
	gfx::Font *font;
};

class Widget_Window : public Widget
{
public:
	Widget_Window(int w, int h);
	Widget_Window(float percent_w, float percent_h);
	Widget_Window(int w, float percent_h);
	Widget_Window(float percent_w, int h);
	Widget_Window(TGUI_Widget::Fit fit, int other);
	Widget_Window(TGUI_Widget::Fit fit, float percent_other);
	virtual ~Widget_Window();

	void draw();

	void set_image(gfx::Image *image);
	void set_alpha(float alpha);

protected:
	void start();

	gfx::Image *image;
	float alpha;
};

} // End namespace gui

} // End namespace noo

#endif // NOO_WIDGETS_H
