#ifndef NOO_PIXEL_FONT_H
#define NOO_PIXEL_FONT_H

#include "shim5/main.h"
#include "shim5/font.h"

namespace noo {

namespace gfx {

class Image;

class SHIM5_EXPORT Pixel_Font : public Font
{
public:
	Pixel_Font(std::string name);
	virtual ~Pixel_Font();

	bool cache_glyphs(std::string text);
	void clear_cache();

	int get_height();

	void flush_vertex_cache();
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_PIXEL_FONT_H
