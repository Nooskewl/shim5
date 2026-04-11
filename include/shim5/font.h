#ifndef NOO_FONT_H
#define NOO_FONT_H

#include "shim5/main.h"

namespace noo {

namespace gfx {

class Image;

ALIGN(16) Font {
public:
	enum Shadow_Type {
		NO_SHADOW = 0,
		DROP_SHADOW,
		FULL_SHADOW
	};

	static std::string strip_codes(std::string text, bool strip_colour_codes, bool strip_wrap_codes, bool strip_extra_glyph_codes);
	static void static_start();
	static void release_all();

	SHIM5_EXPORT Font();
	SHIM5_EXPORT virtual ~Font();

	SHIM5_EXPORT virtual bool cache_glyphs(std::string text) = 0;
	SHIM5_EXPORT virtual void clear_cache() = 0;

	SHIM5_EXPORT virtual int get_text_width(std::string text, bool interpret_colour_codes = true, bool interpret_extra_glyphs = true);
	SHIM5_EXPORT virtual int get_height() = 0;

	// If interpret_colour_codes is true, this returns the last colour used. Otherwise, it returns the colour passed in.
	SHIM5_EXPORT SDL_Color draw(SDL_Color colour, std::string text, util::Point<float> dest_position, bool interpret_colour_codes = true, bool centre = false, bool interpret_extra_glyphs = true, bool rtl = false);

	// Returns number of characters drawn, plus whether or not it filled the max in bool &full
	SHIM5_EXPORT int draw_wrapped(SDL_Color colour, std::string text, util::Point<float> dest_position, int w, int line_height, int max_lines, int elapsed, int delay, bool dry_run, bool &full, int &num_lines, int &width, bool interpret_colour_codes = true, bool centre = false, int first_line_indent = 0, bool interpret_extra_glyphs = true);

	SHIM5_EXPORT void add_extra_glyph(int code, Image *image); // for @00 images
	SHIM5_EXPORT void remove_extra_glyph(int code);
	SHIM5_EXPORT void set_extra_glyph_offset(util::Point<float> offset);

	SHIM5_EXPORT void enable_shadow(SDL_Color shadow_colour, Shadow_Type shadow_type);
	SHIM5_EXPORT void disable_shadow();

	// For 16 byte alignment to make glm::mat4 able to use SIMD
	
protected:
	static std::vector<Font *> loaded_fonts;

	struct Glyph {
		util::Point<int> position;
		util::Size<int> size;
		int sheet;
	};

	std::map<Uint32, Glyph *> glyphs;

	std::vector<Image *> sheets;

	int size_w;
	int size;

	std::map<int, Image *> extra_glyphs;
	util::Point<float> extra_glyph_offset;

	SDL_Color shadow_colour;
	Shadow_Type shadow_type;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_FONT_H
