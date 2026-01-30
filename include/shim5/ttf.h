#ifndef NOO_TTF_H
#define NOO_TTF_H

#include "shim5/main.h"
#include "shim5/font.h"

namespace noo {

namespace gfx {

class Shader;
class Image;

class TTF : public Font
{
public:
	static const int DEFAULT_SHEET_SIZE = 4096;

	SHIM5_EXPORT TTF(std::string filename, int size, int sheet_size = DEFAULT_SHEET_SIZE, bool load_from_filesystem = false);
	SHIM5_EXPORT virtual ~TTF();

	SHIM5_EXPORT bool cache_glyphs(std::string text);
	SHIM5_EXPORT void clear_cache();

	SHIM5_EXPORT int get_height();

	SHIM5_EXPORT void set_smooth(bool smooth);
	
private:
	int get_sheet_size();
	void set_sheet_target(int sheet_num);
	void restore_target();
	Glyph *create_glyph(Uint32 ch, gfx::Image *glyph_image);
	void render_glyph(Glyph *glyph, gfx::Image *glyph_image);
	gfx::Image *load_glyph_image(Uint32 ch);

	TTF_Font *font;

	int num_glyphs;

	int sheet_size;

	glm::mat4 modelview, proj;
	gfx::Shader *old_shader;
	gfx::Image *old_target;

	SDL_IOStream *file;

	int curr_sheet;

	bool smooth;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_TTF_H
