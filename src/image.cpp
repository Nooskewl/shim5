// TGA loader taken from http://paulbourke.net/dataformats/tga/

#include "shim5/gfx.h"
#include "shim5/image.h"
#include "shim5/json.h"
#include "shim5/shader.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/vertex_cache.h"
#include "shim5/util.h"

#include "shim5/internal/gfx.h"

#include <png.h>

using namespace noo;

static inline unsigned char *pixel_ptr(unsigned char *p, int n, bool flip, int w, int h)
{
	if (flip) {
		int x = n % w;
		int y = n / w;
		return p + (w * 4) * (h-1) - (y * w * 4) +  x * 4;
	}
	else {
		return p + n * 4;
	}
}

static inline void pixel_xy(int n, int w, int *x, int *y)
{
	*x = n % w;
	*y = n / w;
}

static inline void maybe_resize_bounds(util::Point<int> &opaque_topleft, util::Point<int> &opaque_bottomright, int x, int y)
{
	if (opaque_topleft.x == -1 || x < opaque_topleft.x) {
		opaque_topleft.x = x;
	}
	if (opaque_topleft.y == -1 || y < opaque_topleft.y) {
		opaque_topleft.y = y;
	}
	if (opaque_bottomright.x == -1 || x > opaque_bottomright.x) {
		opaque_bottomright.x = x;
	}
	if (opaque_bottomright.y == -1 || y > opaque_bottomright.y) {
		opaque_bottomright.y = y;
	}
}

namespace noo {

namespace gfx {

static std::map<std::string, image_loader> image_loaders;
static std::map<std::string, image_saver> image_savers;

std::vector<Image::Internal *> Image::loaded_images;

static GLuint bound_fbo;
bool Image::dumping_colours;
bool Image::keep_data;
bool Image::save_rle;
bool Image::ignore_palette;
bool Image::create_depth_buffer;
bool Image::create_stencil_buffer;
bool Image::premultiply_alpha;
bool Image::save_rgba;
bool Image::save_palettes;

void Image::register_image_loader(std::string ext, image_loader func)
{
	image_loaders[ext] = func;
}

void Image::register_image_saver(std::string ext, image_saver func)
{
	image_savers[ext] = func;
}

void Image::static_start()
{
	register_image_loader("png", read_png);
	register_image_saver("png", save_png);
	register_image_loader("tga", read_tga);
	register_image_saver("tga", save_tga);

	bound_fbo = 0;
	
	util::JSON::Node *root = shim::shim_json->get_root();

	dumping_colours = root->get_nested_bool("shim>gfx>image>dumping_colours", &dumping_colours, false);
	keep_data = root->get_nested_bool("shim>gfx>image>keep_data", &keep_data, false);
	save_rle = root->get_nested_bool("shim>gfx>image>save_rle", &save_rle, true);
	ignore_palette = root->get_nested_bool("shim>gfx>image>ignore_palette", &ignore_palette, false);
	create_depth_buffer = root->get_nested_bool("shim>gfx>image>create_depth_buffer", &create_depth_buffer, true);
	create_stencil_buffer = root->get_nested_bool("shim>gfx>image>create_stencil_buffer", &create_stencil_buffer, false);
	premultiply_alpha = root->get_nested_bool("shim>gfx>image>premultiply_alpha", &premultiply_alpha, true);
	save_rgba = root->get_nested_bool("shim>gfx>image>save_rgba", &save_rgba, true);
	save_palettes = root->get_nested_bool("shim>gfx>image>save_palettes", &save_palettes, true);
}

void Image::release_all(bool include_managed)
{
	util::infomsg("Releasing %d textures...\n", loaded_images.size());
	for (size_t i = 0; i < loaded_images.size(); i++) {
		loaded_images[i]->release();
	}
}

void Image::reload_all(bool include_managed)
{
	for (size_t i = 0; i < loaded_images.size(); i++) {
		loaded_images[i]->reload(false);
	}
}

int Image::get_unfreed_count()
{
	for (size_t i = 0; i < loaded_images.size(); i++) {
		util::infomsg("Unfreed: %s.\n", loaded_images[i]->filename.c_str());
	}
	return (int)loaded_images.size();
}

void Image::audit()
{
}

/* loadpng, Allegro wrapper routines for libpng
 * by Peter Wang (tjaden@users.sf.net).
 *
 * Modified for use in Shim.
 */

static void user_error_fn(png_structp png_ptr, png_const_charp message)
{
   jmp_buf *jmpbuf = (jmp_buf *)png_get_error_ptr(png_ptr);
   (void)message;
   longjmp(*jmpbuf, 1);
}

static void read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
    SDL_IOStream *f = (SDL_IOStream *)png_get_io_ptr(png_ptr);
    if ((png_uint_32)SDL_ReadIO(f, data, length) != length)
	png_error(png_ptr, "read error (loadpng calling pack_fread)");
}

#define PNG_BYTES_TO_CHECK 4

static int check_if_png(SDL_IOStream *fp)
{
    unsigned char buf[PNG_BYTES_TO_CHECK];

    if (SDL_ReadIO(fp, buf, PNG_BYTES_TO_CHECK) != PNG_BYTES_TO_CHECK)
	return 0;

    return (png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK) == 0);
}

unsigned char *Image::read_png(std::string filename, util::Size<int> &out_size, SDL_Color *out_palette, util::Point<int> *opaque_topleft, util::Point<int> *opaque_bottomright, bool *has_alpha, bool load_from_filesystem)
{
	SDL_Color tmppal[256];

	SDL_IOStream *fp;
	if (load_from_filesystem) {
		fp = SDL_IOFromFile(filename.c_str(), "rb");
	}
	else {
		fp = util::open_file(filename, 0);
	}

	if (fp == 0) {
		return 0;
	}

	jmp_buf jmpbuf;
	png_structp png_ptr;
	png_infop info_ptr;

	if (!check_if_png(fp)) {
		if (load_from_filesystem) {
			SDL_CloseIO(fp);
		}
		else {
			util::close_file(fp);
		}
		return NULL;
	}

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also supply the
	 * the compiler header file version, so that we know if the application
	 * was compiled with a compatible version of the library.
	 */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					 (void *)NULL, NULL, NULL);
	if (!png_ptr) {
		if (load_from_filesystem) {
			SDL_CloseIO(fp);
		}
		else {
			util::close_file(fp);
		}
		return NULL;
	}

	/* Allocate/initialize the memory for image information. */
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		if (load_from_filesystem) {
			SDL_CloseIO(fp);
		}
		else {
			util::close_file(fp);
		}
		return NULL;
	}

	/* Set error handling. */
	if (setjmp(jmpbuf)) {
		/* Free all of the memory associated with the png_ptr and info_ptr */
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		/* If we get here, we had a problem reading the file */
		if (load_from_filesystem) {
			SDL_CloseIO(fp);
		}
		else {
			util::close_file(fp);
		}
		return NULL;
	}
	png_set_error_fn(png_ptr, jmpbuf, user_error_fn, NULL);

	/* Use Allegro packfile routines. */
	png_set_read_fn(png_ptr, fp, (png_rw_ptr)read_data);

	/* We have already read some of the signature. */
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	/* Really load the image now. */
	png_uint_32 width, height, rowbytes;
	int bit_depth, color_type, interlace_type;
	int number_passes, pass;

	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).
	 */
	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		 &interlace_type, NULL, NULL);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	png_set_expand(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb (png_ptr);
	}	

	/* Adds a full alpha channel if there is transparency information
	 * in a tRNS chunk. */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png_ptr);
	}

	/* Convert 16-bits per colour component to 8-bits per colour component. */
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	else if (bit_depth < 8) {
		png_set_packing(png_ptr);
	}

	/* Convert grayscale to RGB triplets */
	if ((color_type == PNG_COLOR_TYPE_GRAY) ||
		(color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
		png_set_gray_to_rgb(png_ptr);

	/* Turn on interlace handling. */
	number_passes = png_set_interlace_handling(png_ptr);
	
	/* Call to gamma correct and add the background to the palette
	 * and update info structure.
	 */
	png_read_update_info(png_ptr, info_ptr);
	
	/* Palettes. */
	if (color_type & PNG_COLOR_MASK_PALETTE) {
		int num_palette, i;
		png_colorp palette;

		if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
			/* We don't actually dither, we just copy the palette. */
			for (i = 0; ((i < num_palette) && (i < 256)); i++) {
				tmppal[i].r = palette[i].red;
				tmppal[i].g = palette[i].green;
				tmppal[i].b = palette[i].blue;
			}

			for (; i < 256; i++)
				tmppal[i].r = tmppal[i].g = tmppal[i].b = 0;
		}
	}

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	unsigned char *bytes = new unsigned char[width*height*4];
	unsigned char *row = new unsigned char[rowbytes];

	/* Read the image, one line at a line (easier to debug!) */
	for (pass = 0; pass < number_passes; pass++) {
		png_uint_32 y;
		for (y = 0; y < height; y++) {
			int yy = (height-1)-y;
			if (color_type == PNG_COLOR_TYPE_RGB) {
				png_read_row(png_ptr, row, NULL);
				unsigned char *d = bytes+yy*width*4;
				unsigned char *s = row;
				for (unsigned int i = 0; i < width; i++) {
					int r = *s++;
					int g = *s++;
					int b = *s++;
					*d++ = r;
					*d++ = g;
					*d++ = b;
					*d++ = 255;
				}
			}
			else {
				png_read_row(png_ptr, row, NULL);
				memcpy(bytes+yy*width*4, row, width*4);
			}
		}
	}

	delete[] row;

	/* Read rest of file, and get additional chunks in info_ptr. */
	png_read_end(png_ptr, info_ptr);

	/* Clean up after the read, and free any memory allocated. */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

	if (out_palette != nullptr) {
		memcpy(out_palette, tmppal, 256*4);
	}

	out_size.w = width;
	out_size.h = height;

	util::Point<int> tl, br;
	tl.x = -1;
	tl.y = -1;
	br.x = -1;
	br.y = -1;
	bool alpha = false;

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			unsigned char *p = bytes + y * width * 4 + x * 4;
			int r, g, b, a;
			r = p[0];
			g = p[1];
			b = p[2];
			a = p[3];
			if (a != 255) {
				alpha = true;
				float f = a/255.0f;
				r *= f;
				g *= f;
				b *= f;
				p[0] = r;
				p[1] = g;
				p[2] = b;
			}
			if (a != 0) {
				if (tl.x < 0 || tl.x > (int)x) {
					tl.x = x;
				}
				if (tl.y < 0 || tl.y > (int)y) {
					tl.y = y;
				}
				if (br.x < 0 || br.x < (int)x) {
					br.x = x;
				}
				if (br.y < 0 || br.y < (int)y) {
					br.y = y;
				}
			}
			p += 4;
		}
	}

	if (opaque_topleft != nullptr) {
		*opaque_topleft = tl;
	}

	if (opaque_bottomright != nullptr) {
		*opaque_bottomright = br;
	}

	if (has_alpha != nullptr) {
		*has_alpha = alpha;
	}
		
	if (load_from_filesystem) {
		SDL_CloseIO(fp);
	}
	else {
		util::close_file(fp);
	}

	return bytes;
}

bool Image::save_png(std::string filename, unsigned char *data, util::Size<int> size, bool _save_rgba)
{
	FILE *fp;
	errno_t err = fopen_s(&fp, filename.c_str(), "wb");
	if (err) {
		return false;
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		return false;
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		return false;
	}

	if (setjmp(png_jmpbuf(png))) {
		return false;
	}

	png_init_io(png, fp);

	png_set_IHDR(
		png,
		info,
		size.w, size.h,
		8,
		PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);

	png_write_info(png, info);

	if (_save_rgba == false) {
		png_set_filler(png, 0, PNG_FILLER_AFTER);
	}

	if (!data) {
		return false;
	}

	unsigned char **row_pointers = new unsigned char *[size.h];
	for (int i = 0; i < size.h; i++) {
		row_pointers[i] = data + i * (size.w * 4);
	}

	png_write_image(png, row_pointers);
	png_write_end(png, NULL);

	fclose(fp);

	png_destroy_write_struct(&png, &info);

	delete[] row_pointers;

	return true;
}

// returns true if pixel is transparent
bool Image::merge_bytes(unsigned char *pixel, unsigned char *p, int bytes, TGA_Header *header, bool *alpha)
{
	if (header->colourmaptype == 1) {
		SDL_Color *colour;
		if (ignore_palette) {
			colour = &shim::palette[*p];
		}
		else {
			colour = &header->palette[*p];
		}
		// Magic pink
		// Paletted
		if (colour->r == 255 && colour->g == 0 && colour->b == 255) {
			// transparent
			*pixel++ = 0;
			*pixel++ = 0;
			*pixel++ = 0;
			*pixel++ = 0;
			return true;
		}
		else {
			*pixel++ = colour->r;
			*pixel++ = colour->g;
			*pixel++ = colour->b;
			*pixel++ = 255;
		}
		*alpha = false;
	}
	else {
		if (bytes == 4) {
			if (premultiply_alpha) {
				float a = p[3] / 255.0f;
				*pixel++ = (unsigned char)(p[2] * a);
				*pixel++ = (unsigned char)(p[1] * a);
				*pixel++ = (unsigned char)(p[0] * a);
			}
			else {
				*pixel++ = p[2];
				*pixel++ = p[1];
				*pixel++ = p[0];
			}
			*pixel++ = p[3];
			if (p[3] != 255 && p[3] != 0) {
				*alpha = true;
			}
			else {
				*alpha = false;
			}
			return p[3] == 0;
		}
		else if (bytes == 3) {
			*pixel++ = p[2];
			*pixel++ = p[1];
			*pixel++ = p[0];
			*pixel++ = 255;
			*alpha = false;
		}
		else if (bytes == 2) {
			*pixel++ = (p[1] & 0x7c) << 1;
			*pixel++ = ((p[1] & 0x03) << 6) | ((p[0] & 0xe0) >> 2);
			*pixel++ = (p[0] & 0x1f) << 3;
			*pixel++ = (p[1] & 0x80) ? 255 : 0;
			*alpha = false;
			return (p[1] & 0x80) == 0;
		}
	}

	return false;
}

unsigned char *Image::read_tga(std::string filename, util::Size<int> &out_size, SDL_Color *out_palette, util::Point<int> *opaque_topleft, util::Point<int> *opaque_bottomright, bool *has_alpha, bool load_from_filesystem)
{
	SDL_IOStream *file;
	if (load_from_filesystem) {
		file = SDL_IOFromFile(filename.c_str(), "rb");
	}
	else {
		file = util::open_file(filename, 0);
	}

	if (file == 0) {
		return 0;
	}

	int n = 0, i = 0, j;
	int bytes2read;
	unsigned char p[5];
	TGA_Header header;
	unsigned char *pixels;
	int pixel_x, pixel_y;
	bool alpha;

	/* Display the header fields */
	header.idlength = util::SDL_fgetc(file);
	header.colourmaptype = util::SDL_fgetc(file);
	header.datatypecode = util::SDL_fgetc(file);
	SDL_ReadU16LE(file, &header.colourmaporigin);
	SDL_ReadU16LE(file, &header.colourmaplength);
	header.colourmapdepth = util::SDL_fgetc(file);
	SDL_ReadU16LE(file, &header.x_origin);
	SDL_ReadU16LE(file, &header.y_origin);
	SDL_ReadU16LE(file, &header.width);
	SDL_ReadU16LE(file, &header.height);
	header.bitsperpixel = util::SDL_fgetc(file);
	header.imagedescriptor = util::SDL_fgetc(file);

	int w, h;
	out_size.w = w = header.width;
	out_size.h = h = header.height;

	if (has_alpha) {
		*has_alpha = false;
	}

	try {
		/* Allocate space for the image */
		if ((pixels = new unsigned char[header.width*header.height*4]) == 0) {
			throw util::MemoryError("malloc of image failed");
		}

		/* What can we handle */
		if (header.datatypecode != 1 && header.datatypecode != 2 && header.datatypecode != 9 && header.datatypecode != 10) {
			throw util::LoadError("can only handle image type 1, 2, 9 and 10");
		}
		if (header.bitsperpixel != 8 && header.bitsperpixel != 16 && header.bitsperpixel != 24 && header.bitsperpixel != 32) {
			throw util::LoadError("can only handle pixel depths of 8, 16, 24 and 32");
		}
		if (header.colourmaptype != 0 && header.colourmaptype != 1) {
			throw util::LoadError("can only handle colour map types of 0 and 1");
		}

		/* Skip over unnecessary stuff */
		SDL_SeekIO(file, header.idlength, SDL_IO_SEEK_CUR);

		/* Read the palette if there is one */
		if (header.colourmaptype == 1) {
			if (header.colourmapdepth != 24) {
				throw util::LoadError("can't handle anything but 24 bit palettes");
			}
			if (header.bitsperpixel != 8) {
				throw util::LoadError("can only read 8 bpp paletted images");
			}
			int skip = header.colourmaporigin * (header.colourmapdepth / 8);
			SDL_SeekIO(file, skip, SDL_IO_SEEK_CUR);
			// We can only read 256 colour palettes max, skip the rest
			int size = MIN(header.colourmaplength-skip, 256);
			skip = (header.colourmaplength - size) * (header.colourmapdepth / 8);
			for (i = 0; i < size; i++) {
				header.palette[i].b = util::SDL_fgetc(file);
				header.palette[i].g = util::SDL_fgetc(file);
				header.palette[i].r = util::SDL_fgetc(file);
			}
			SDL_SeekIO(file, skip, SDL_IO_SEEK_CUR);
		}
		else {
			// Skip the palette on truecolour images
			SDL_SeekIO(file, (header.colourmapdepth / 8) * header.colourmaplength, SDL_IO_SEEK_CUR);
		}

		bool flip = (header.imagedescriptor & 0x20) != 0;

		/* Read the image */
		bytes2read = header.bitsperpixel / 8;
		while (n < header.width * header.height) {
			if (header.datatypecode == 1 || header.datatypecode == 2) {                     /* Uncompressed */
				if (SDL_ReadIO(file, p, bytes2read) != (size_t)bytes2read) {
					delete[] pixels;
					throw util::LoadError("unexpected end of file at pixel " + util::itos(i));
				}
				if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), p, bytes2read, &header, &alpha) == false) {
					if (opaque_topleft != 0 && opaque_bottomright != 0) {
						pixel_xy(n, w, &pixel_x, &pixel_y);
						maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
					}
				}
				n++;
				if (alpha && has_alpha) {
					*has_alpha = true;
				}
			}
			else if (header.datatypecode == 9 || header.datatypecode == 10) {             /* Compressed */
				if (SDL_ReadIO(file, p, bytes2read+1) != (size_t)bytes2read+1) {
					delete[] pixels;
					throw util::LoadError("unexpected end of file at pixel " + util::itos(i));
				}
				j = p[0] & 0x7f;
				if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), &(p[1]), bytes2read, &header, &alpha) == false) {
					if (opaque_topleft != 0 && opaque_bottomright != 0) {
						pixel_xy(n, w, &pixel_x, &pixel_y);
						maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
					}
				}
				n++;
				if (alpha && has_alpha) {
					*has_alpha = true;
				}
				if (p[0] & 0x80) {         /* RLE chunk */
					for (i = 0; i < j; i++) {
						if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), &(p[1]), bytes2read, &header, &alpha) == false) {
							if (opaque_topleft != 0 && opaque_bottomright != 0) {
								pixel_xy(n, w, &pixel_x, &pixel_y);
								maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
							}
						}
						n++;
						if (alpha && has_alpha) {
							*has_alpha = true;
						}
					}
				}
				else {                   /* Normal chunk */
					for (i = 0; i < j; i++) {
						if (SDL_ReadIO(file, p, bytes2read) != (size_t)bytes2read) {
							delete[] pixels;
							throw util::LoadError("unexpected end of file at pixel " + util::itos(i));
						}
						if (merge_bytes(pixel_ptr(pixels, n, flip, w, h), p, bytes2read, &header, &alpha) == false) {
							if (opaque_topleft != 0 && opaque_bottomright != 0) {
								pixel_xy(n, w, &pixel_x, &pixel_y);
								maybe_resize_bounds(*opaque_topleft, *opaque_bottomright, pixel_x, pixel_y);
							}
						}
						n++;
						if (alpha && has_alpha) {
							*has_alpha = true;
						}
					}
				}
			}
		}
	}
	catch (util::Error &) {
		util::close_file(file);
		throw;
	}

	util::close_file(file);

	if (out_palette != 0) {
		memcpy(out_palette, header.palette, 256 * 3);
	}

	return pixels;
}

bool Image::save_tga(std::string filename, unsigned char *loaded_data, util::Size<int> size, bool _save_rgba)
{
	unsigned char header[] = {
		(unsigned char)0x00, // idlength
		(unsigned char)0x01, // colourmap type 1 == palette
		(unsigned char)(save_rle ? 0x09 : 0x01),
		(unsigned char)0x00, (unsigned char)0x00, // colourmap origin (little endian)
		(unsigned char)0x00, save_palettes ? (unsigned char)0x01 : (unsigned char)0x00, // # of palette entries
		(unsigned char)0x18, // colourmap depth
		(unsigned char)0x00, (unsigned char)0x00, // x origin
		(unsigned char)0x00, (unsigned char)0x00, // y origin
		(unsigned char)(size.w & 0xff), (unsigned char)((size.w >> 8) & 0xff), // width
		(unsigned char)(size.h & 0xff), (unsigned char)((size.h >> 8) & 0xff), // height
		(unsigned char)0x08, // bits per pixel
		(unsigned char)0x00 // image descriptor
	};

	if (_save_rgba) {
		header[1] = 0;
		header[2] = 2;
		header[5] = 0;
		header[6] = 0;
		header[7] = 0;
		header[16] = 32;
		header[17] = 8;
	}

	int header_size = 18;

	SDL_IOStream *file = SDL_IOFromFile(filename.c_str(), "wb");
	if (file == 0) {
		throw util::Error("Couldn't open " + filename + " for writing");
	}

	for (int i = 0; i < header_size; i++) {
		if (util::SDL_fputc(header[i], file) == EOF) {
			throw util::Error("Write error writing to " + filename);
		}
	}

	if (_save_rgba == false) {
		if (save_palettes) {
			for (int i = 0; i < 256; i++) {
				if (util::SDL_fputc(shim::palette[i].b, file) == EOF) {
					throw util::Error("Write error writing to " + filename);
				}
				if (util::SDL_fputc(shim::palette[i].g, file) == EOF) {
					throw util::Error("Write error writing to " + filename);
				}
				if (util::SDL_fputc(shim::palette[i].r, file) == EOF) {
					throw util::Error("Write error writing to " + filename);
				}
			}
		}

		#define R(n) *(pixel_ptr(loaded_data, n, false, size.w, size.h)+0)
		#define G(n) *(pixel_ptr(loaded_data, n, false, size.w, size.h)+1)
		#define B(n) *(pixel_ptr(loaded_data, n, false, size.w, size.h)+2)

		if (save_rle) {
			for (int i = 0; i < size.w * size.h;) {
				int j, count;
				int next_line = i - (i % size.w) + size.w - 1;
				for (j = i, count = 0; j < size.w * size.h - 1 && j < next_line && count < 127; j++, count++) {
					if (R(j) != R(j+1) || G(j) != G(j+1) || B(j) != B(j+1)) {
						break;
					}
				}
				int run_length = j - i + 1;
				if (run_length > 1) {
					util::SDL_fputc((run_length-1) | 0x80, file);
					util::SDL_fputc(find_colour_in_palette(&R(i)), file);
				}
				else {
					for (j = i, count = 0; j < size.w * size.h - 1 && j < next_line && count < 127; j++, count++) {
						if (R(j) == R(j+1) && G(j) == G(j+1) && B(j) == B(j+1)) {
							break;
						}
					}
					run_length = j - i + 1;
					// I noticed PSP never stores a non-run of 2 pixels, and this saves some space usually, so we do the same
					if (run_length == 2) {
						run_length--;
					}
					util::SDL_fputc((run_length-1), file);
					util::SDL_fputc(find_colour_in_palette(&R(i)), file);
					for (j = 0; j < run_length-1; j++) {
						util::SDL_fputc(find_colour_in_palette(&R(i+j+1)), file);
					}
				}
				i += run_length;
			}
		}
		else {
			for (int i = 0; i < size.w * size.h; i++) {
				util::SDL_fputc(find_colour_in_palette(&R(i)), file);
			}
		}
	}
	else {
		unsigned char *tmp = new unsigned char[size.w * size.h * 4];
		unsigned char *p = tmp;
		unsigned char *p2 = loaded_data;
		for (int i = 0; i < size.w * size.h; i++) {
			unsigned char r = *p2++;
			unsigned char g = *p2++;
			unsigned char b = *p2++;
			unsigned char a = *p2++;
			*p++ = b;
			*p++ = g;
			*p++ = r;
			*p++ = a;
		}
		SDL_WriteIO(file, tmp, size.w * size.h * 4);
		delete[] tmp;
	}

	return true;
}

unsigned char *Image::read_backbuffer(bool include_black_bars, int *out_w, int *out_h)
{
	int x, y, w, h;

	if (include_black_bars) {
		x = 0;
		y = 0;
		w = shim::real_screen_size.w;
		h = shim::real_screen_size.h;
	}
	else {
		x = shim::screen_offset.x;
		y = shim::screen_offset.y;
		w = shim::screen_size.w * shim::scale;
		h = shim::screen_size.h * shim::scale;
	}

	if (out_w != nullptr) {
		*out_w = w;
	}
	if (out_h != nullptr) {
		*out_h = h;
	}

	unsigned char *buf = new unsigned char[w * h * 4];

	glReadPixels_ptr(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	PRINT_GL_ERROR("glReadPixels\n");
	// The image is flipped, so flip it
	unsigned char *line = new unsigned char[w * 4];
	for (int i = 0; i < h / 2; i++) {
		memcpy(line, buf+i*(w*4), w*4);
		memcpy(buf+i*(w*4), buf+(h-1-i)*w*4, w*4);
		memcpy(buf+(h-1-i)*(w*4), line, w*4);
	}
	delete[] line;
	
	return buf;
}

unsigned char *Image::read_texture(gfx::Image *image)
{
	unsigned char *buf = new unsigned char[image->size.w*image->size.h*4];

	glActiveTexture_ptr(GL_TEXTURE0);
	PRINT_GL_ERROR("glActiveTexture\n");

	glBindTexture_ptr(GL_TEXTURE_2D, image->internal->texture);
	PRINT_GL_ERROR("glBindTexture\n");

	glGetTexImage_ptr(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	return buf;
}

Image::Image(std::string filename, bool is_absolute_path, bool load_from_filesystem) :
	batching(false),
	flipped(true),
	data_to_destroy(0)
{
	if (is_absolute_path == false && load_from_filesystem == false) {
		filename = "gfx/images/" + filename;
	}

	this->filename = filename;

	reload(load_from_filesystem);
}

Image::Image(Uint8 *data, util::Size<int> size, bool destroy_data) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(true)
{
	if (destroy_data) {
		data_to_destroy = data;
	}
	else {
		data_to_destroy = 0;
	}

	try {
		internal = new Internal(data, size);
	}
	catch (util::Error &) {
		delete[] data;
		throw;
	}

	internal->has_alpha = false;
	Uint8 *p = data;
	for (int i = 0; i < size.w * size.h; i++) {
		if (p[3] != 255) {
			internal->has_alpha = true;
			break;
		}
		p += 4;
	}
}

Image::Image(SDL_Surface *surface) :
	filename("--NOT LOADED--"),
	batching(false),
	flipped(true),
	data_to_destroy(0)
{
	unsigned char *pixels;
	unsigned char *packed = 0;
	SDL_Surface *tmp = 0;
	SDL_Surface *fetch;

	if (surface->format == SDL_PIXELFORMAT_ABGR8888) {
		fetch = surface;
	}
	else {
		tmp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
		if (tmp == 0) {
			throw util::Error("SDL_ConvertSurface returned 0");
		}
		fetch = tmp;
	}

	if (fetch->pitch != fetch->w * 4) {
		// must be packed
		packed = new unsigned char[fetch->w * 4 * fetch->h];
		for (int y = 0; y < fetch->h; y++) {
			memcpy(packed + (y * fetch->w * 4), ((unsigned char *)fetch->pixels) + y * fetch->pitch, fetch->w * 4);
		}
		pixels = packed;
	}
	else {
		pixels = (unsigned char *)fetch->pixels;
	}

	size = {surface->w, surface->h};

	for (int y = 0; y < size.h; y++) {
		for (int x = 0; x < size.w; x++) {
			unsigned char *p = (unsigned char *)(pixels + y * (size.w * 4) + x * 4);
				float a = p[3] / 255.0f;
				p[0] *= a;
				p[1] *= a;
				p[2] *= a;
		}
	}

	try {
		internal = new Internal(pixels, size);
	}
	catch (util::Error &) {
		if (tmp) SDL_DestroySurface(tmp);
		throw;
	}

	internal->has_alpha = false;
	Uint8 *p = pixels;
	for (int i = 0; i < size.w * size.h; i++) {
		if (p[3] != 255) {
			internal->has_alpha = true;
			break;
		}
		p += 4;
	}

	if (tmp) {
		SDL_DestroySurface(tmp);
	}

	if (packed) {
		delete[] packed;
	}
}

Image::Image(util::Size<int> size) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(false),
	data_to_destroy(0)
{
	unsigned char *pixels = new unsigned char[size.w * size.h * 4];
	memset(pixels, 0, size.w * size.h * 4);

	try {
		internal = new Internal(pixels, size, true); // support render to texture
	}
	catch (util::Error &) {
		delete[] pixels;
		throw;
	}

	internal->has_alpha = true;

	delete[] pixels;
}

Image::Image(util::Size<int> size, unsigned char *pixels) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(false),
	data_to_destroy(0)
{
	try {
		internal = new Internal(pixels, size, true); // support render to texture
	}
	catch (util::Error &) {
		delete[] pixels;
		throw;
	}

	internal->has_alpha = true;
}

Image::Image(Image *parent, util::Point<int> offset, util::Size<int> size) :
	filename("--NOT LOADED--"),
	size(size),
	batching(false),
	flipped(true),
	data_to_destroy(0)
{
	internal = new Internal;
	internal->parent = parent;
	internal->offset = offset;

	internal->has_alpha = parent->internal->has_alpha; // FIXME: not always true

	Image::Internal *parent_internal = parent->internal;

	for (int y = 0; y < size.h; y++) {
		for (int x = 0; x < size.w; x++) {
			if (parent_internal->is_transparent(util::Point<int>(x, y) + offset) == false) {
				maybe_resize_bounds(internal->opaque_topleft, internal->opaque_bottomright, x, y);
			}
		}
	}
}

Image::~Image()
{
	release();
	delete[] data_to_destroy;
}

void Image::release()
{
	if (filename == "--NOT LOADED--") {
		internal->unbind();
		delete internal;
		internal = 0;
		return;
	}

	for (size_t i = 0; i < loaded_images.size(); i++) {
		Internal *ii = loaded_images[i];
		if (ii->filename == filename) {
			ii->refcount--;
			if (ii->refcount == 0) {
				internal->unbind();
				loaded_images.erase(loaded_images.begin()+i);
				delete ii;
				return;
			}
		}
	}
}

void Image::reload(bool load_from_filesystem)
{
	if (filename == "--NOT LOADED--") {
		return;
	}

	for (size_t i = 0; i < loaded_images.size(); i++) {
		Internal *ii = loaded_images[i];
		if (ii->filename == filename) {
			ii->refcount++;
			internal = ii;
			size = internal->size;
			return;
		}
	}

	internal = new Internal(filename, keep_data, false, load_from_filesystem);
	size = internal->size;
	loaded_images.push_back(internal);
}

unsigned char Image::find_colour_in_palette(unsigned char *p)
{
	if (p[3] == 0) {
		return 0;
	}

	for (unsigned int i = 0; i < 256; i++) {
		if (p[0] == shim::palette[i].r && p[1] == shim::palette[i].g && p[2] == shim::palette[i].b) {
			return i;
		}
	}

	util::errormsg("Error: colour %d,%d,%d not found!\n", p[0], p[1], p[2]);

	return 0;
}

unsigned char *Image::load_image(std::string filename, util::Size<int> &out_size, SDL_Color *out_palette, util::Point<int> *opaque_topleft, util::Point<int> *opaque_bottomright, bool *has_alpha, bool load_from_filesystem)
{
	std::pair<std::string, image_loader> p;
	std::map<std::string, image_loader>::iterator it;
	size_t loc = filename.rfind('.');
	if (loc == std::string::npos) {
		return nullptr;
	}
	std::string ext = filename.substr(loc+1);
	ext = util::lowercase(ext);
	it = image_loaders.find(ext);
	if (it == image_loaders.end()) {
		return nullptr;
	}
	return image_loaders[ext](filename, out_size, out_palette, opaque_topleft, opaque_bottomright, has_alpha, load_from_filesystem);
}

bool Image::save_image(std::string filename, unsigned char *loaded_data, util::Size<int> size, bool _save_rgba)
{
	std::pair<std::string, image_saver> p;
	std::map<std::string, image_saver>::iterator it;
	size_t loc = filename.rfind('.');
	if (loc == std::string::npos) {
		return false;
	}
	std::string ext = filename.substr(loc+1);
	ext = util::lowercase(ext);
	it = image_savers.find(ext);
	if (it == image_savers.end()) {
		return false;
	}
	return image_savers[ext](filename, loaded_data, size, _save_rgba);
}

bool Image::save(std::string filename)
{
	bool _save_rgba = internal->has_alpha || save_rgba; // FIXME: should be able to force NOT saving RGBA

	if (filename.find(".png") != std::string::npos) {
		return save_png(filename, internal->loaded_data, size, _save_rgba);
	}
	else {
		return save_tga(filename, internal->loaded_data, size, _save_rgba);
	}
}

void Image::set_target()
{
	bound_fbo = internal->fbo;
	glBindFramebuffer_ptr(GL_FRAMEBUFFER, internal->fbo);
	glViewport_ptr(0, 0, size.w, size.h);
	glDisable_ptr(GL_SCISSOR_TEST);

	glm::mat4 modelview = glm::mat4();
	glm::mat4 proj = glm::ortho(0.0f, (float)size.w, (float)size.h, 0.0f);
	set_matrices(modelview, proj);
	update_projection();
}

void Image::release_target()
{
	bound_fbo = 0;
	glBindFramebuffer_ptr(GL_FRAMEBUFFER, 0);
	set_screen_size(shim::real_screen_size); // this sets the viewport and scissor, updates projection
}

void Image::get_bounds(util::Point<int> &topleft, util::Point<int> &bottomright)
{
	topleft = internal->opaque_topleft;
	bottomright = internal->opaque_bottomright;
}

void Image::set_bounds(util::Point<int> topleft, util::Point<int> bottomright)
{
	internal->opaque_topleft = topleft;
	internal->opaque_bottomright = bottomright;
}

void Image::destroy_data()
{
	if (internal != 0) {
		internal->destroy_data();
	}
}

Image *Image::get_root()
{
	gfx::Image *root = this;
	while (root && root->internal->parent != 0) {
		root = root->internal->parent;
	}
	return root;
}

unsigned char *Image::get_loaded_data()
{
	return internal->loaded_data;
}

void Image::start_batch(bool repeat)
{
	Image *root = get_root();
	Vertex_Cache::instance()->start(root, repeat);
	root->batching = true;
}

void Image::end_batch()
{
	Vertex_Cache::instance()->end();
	get_root()->batching = false;
}

void Image::stretch_region_tinted_repeat(SDL_Color tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;

	int wt = dest_size.w / source_size.w;
	if (dest_size.w % source_size.w != 0) {
		wt++;
	}
	int ht = dest_size.h / source_size.h;
	if (dest_size.h % source_size.h != 0) {
		ht++;
	}

	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();

	int drawn_h = 0;
	for (int y = 0; y < ht; y++) {
		int drawn_w = 0;
		int h = source_size.h;
		if (dest_size.h - drawn_h < h) {
			h = dest_size.h- drawn_h;
		}
		for (int x = 0; x < wt; x++) {
			int w = source_size.w;
			if (dest_size.w - drawn_w < w) {
				w = dest_size.w - drawn_w;
			}
			util::Size<int> sz(w, h);
			Vertex_Cache::instance()->cache(colours, source_position+internal->offset, sz, {dest_position.x + x * source_size.w, dest_position.y + y * source_size.h}, sz, flags);
			drawn_w += w;
		}
		drawn_h += h;
	}

	if (was_batching == false) end_batch();
}

void Image::stretch_region_tinted(SDL_Color tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache(colours, source_position+internal->offset, source_size, dest_position, dest_size, flags);
	if (was_batching == false) end_batch();
}

void Image::stretch_region(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, util::Size<int> dest_size, int flags)
{
	stretch_region_tinted(shim::white, source_position, source_size, dest_position, dest_size, flags);
}

void Image::draw_region_lit_z_range(SDL_Color colours[4], util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags)
{
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache_z_range(colours, source_position+internal->offset, source_size, dest_position, z_top, z_bottom, source_size, flags);
	if (was_batching == false) end_batch();
}

void Image::draw_region_lit_z(SDL_Color colours[4], util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags)
{
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, z, z, flags);
}

void Image::draw_region_tinted_z_range(SDL_Color tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, z_top, z_bottom, flags);
}

void Image::draw_region_tinted_z(SDL_Color tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags)
{
	draw_region_tinted_z_range(tint, source_position, source_size, dest_position, z, z, flags);
}

void Image::draw_region_tinted(SDL_Color tint, util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, 0.0f, 0.0f, flags);
}

void Image::draw_region_z_range(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z_top, float z_bottom, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = shim::white;
	draw_region_lit_z_range(colours, source_position, source_size, dest_position, z_top, z_bottom, flags);
}

void Image::draw_region_z(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, float z, int flags)
{
	draw_region_tinted_z(shim::white, source_position, source_size, dest_position, z, flags);
}

void Image::draw_region(util::Point<float> source_position, util::Size<int> source_size, util::Point<float> dest_position, int flags)
{
	draw_region_z(source_position, source_size, dest_position, 0.0f, flags);
}

void Image::draw_z(util::Point<float> dest_position, float z, int flags)
{
	draw_region_z({0.0f, 0.0f}, size, dest_position, z, flags);
}

void Image::draw_tinted(SDL_Color tint, util::Point<float> dest_position, int flags)
{
	draw_region_tinted(tint, {0.0f, 0.0f}, size, dest_position, flags);
}

void Image::draw(util::Point<float> dest_position, int flags)
{
	draw_z(dest_position, 0.0f, flags);
}

void Image::draw_tinted_rotated(SDL_Color tint, util::Point<float> centre, util::Point<float> dest_position, float angle, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache(colours, centre, internal->offset, size, dest_position, angle, 1.0f, flags);
	if (was_batching == false) end_batch();
}

void Image::draw_tinted_rotated_scaledxy_z(SDL_Color tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale_x, float scale_y, float z, int flags)
{
	SDL_Color colours[4];
	colours[0] = colours[1] = colours[2] = colours[3] = tint;
	bool was_batching = get_root()->batching;
	if (was_batching == false) start_batch();
	Vertex_Cache::instance()->cache_z(colours, centre, internal->offset, size, dest_position, angle, scale_x, scale_y, z, flags);
	if (was_batching == false) end_batch();
}

void Image::draw_tinted_rotated_scaled_z(SDL_Color tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, float z, int flags)
{
	draw_tinted_rotated_scaledxy_z(tint, centre, dest_position, angle, scale, scale, z, flags);
}

void Image::draw_rotated_scaled_z(util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, float z, int flags)
{
	draw_tinted_rotated_scaledxy_z(shim::white, centre, dest_position, angle, scale, scale, z, flags);
}

void Image::draw_tinted_rotated_scaled(SDL_Color tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, int flags)
{
	draw_tinted_rotated_scaled_z(tint, centre, dest_position, angle, scale, 0.0f, flags);
}

void Image::draw_tinted_rotated_scaledxy(SDL_Color tint, util::Point<float> centre, util::Point<float> dest_position, float angle, float scale_x, float scale_y, int flags)
{
	draw_tinted_rotated_scaledxy_z(tint, centre, dest_position, angle, scale_x, scale_y, 0.0f, flags);
}

void Image::draw_rotated(util::Point<float> centre, util::Point<float> dest_position, float angle, int flags)
{
	draw_tinted_rotated(shim::white, centre, dest_position, angle, flags);
}

void Image::draw_rotated_scaled(util::Point<float> centre, util::Point<float> dest_position, float angle, float scale, int flags)
{
	draw_tinted_rotated_scaled(shim::white, centre, dest_position, angle, scale, flags);
}

GLuint Image::get_opengl_texture()
{
	gfx::Image *root = get_root();
	GLuint texture = root == 0 ? 0 : root->internal->texture;
	return texture;
}

bool Image::is_sub_image()
{
	return internal->parent != nullptr;
}

util::Point<int> Image::get_offset()
{
	return internal->offset;
}

void Image::update(unsigned char *pixels)
{
	glActiveTexture_ptr(GL_TEXTURE0);
	PRINT_GL_ERROR("glActiveTexture\n");

	glBindTexture_ptr(GL_TEXTURE_2D, internal->texture);
	PRINT_GL_ERROR("glBindTexture\n");

	glTexImage2D_ptr(GL_TEXTURE_2D, 0, GL_RGBA, size.w, size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	PRINT_GL_ERROR("glTexImage2D\n");

	Shader::rebind_opengl_texture0();
}

//--

Image::Internal::Internal(std::string filename, bool keep_data, bool support_render_to_texture, bool load_from_filesystem) :
	loaded_data(0),
	filename(filename),
	refcount(1),
	has_render_to_texture(support_render_to_texture),
	texture(0),
	depth_buffer(0),
	stencil_buffer(0),
	parent(0),
	offset(0, 0),
	opaque_topleft(-1, -1),
	opaque_bottomright(-1, -1)
{
	this->create_depth_buffer = Image::create_depth_buffer;
	this->create_stencil_buffer = Image::create_stencil_buffer;
	loaded_data = reload(keep_data, load_from_filesystem);
}

Image::Internal::Internal(unsigned char *pixels, util::Size<int> size, bool support_render_to_texture) :
	loaded_data(0),
	size(size),
	has_render_to_texture(support_render_to_texture),
	texture(0),
	depth_buffer(0),
	stencil_buffer(0),
	parent(0),
	offset(0, 0),
	opaque_topleft(-1, -1),
	opaque_bottomright(-1, -1)
{
	this->create_depth_buffer = Image::create_depth_buffer;
	this->create_stencil_buffer = Image::create_stencil_buffer;
	filename = "--NOT LOADED--";
	upload(pixels);
}

Image::Internal::Internal() :
	loaded_data(0),
	has_render_to_texture(false),
	texture(0),
	depth_buffer(0),
	stencil_buffer(0),
	parent(0),
	offset(0, 0),
	opaque_topleft(-1, -1),
	opaque_bottomright(-1, -1)
{
	this->create_depth_buffer = Image::create_depth_buffer;
	this->create_stencil_buffer = Image::create_stencil_buffer;
}

Image::Internal::~Internal()
{
	release();

	delete[] loaded_data;
	loaded_data = 0;
}

void Image::Internal::release()
{
	if (parent) {
		return;
	}

	unbind();

	if (bound_fbo == fbo) {
		bound_fbo = 0;
	}
	if (has_render_to_texture) {
		if (depth_buffer != 0) {
			glDeleteRenderbuffers_ptr(1, &depth_buffer);
			depth_buffer = 0;
		}
		if (stencil_buffer != 0) {
			glDeleteRenderbuffers_ptr(1, &stencil_buffer);
			stencil_buffer = 0;
		}
		if (fbo != 0) {
			glDeleteFramebuffers_ptr(1, &fbo);
			fbo = 0;
		}
	}

	if (texture != 0) {
		glDeleteTextures_ptr(1, &texture);
		PRINT_GL_ERROR("glDeleteTextures\n");
		texture = 0;
	}
}

unsigned char *Image::Internal::reload(bool keep_data, bool load_from_filesystem)
{
	unsigned char *pixels;

	pixels = Image::load_image(filename, size, NULL, &opaque_topleft, &opaque_bottomright, &has_alpha, load_from_filesystem);

	if (pixels == 0) {
		throw util::LoadError("Could not read " + filename);
	}

	try {
		upload(pixels);
	}
	catch (util::Error &) {
		delete[] pixels;
		throw;
	}

	if (keep_data == false) {
		delete[] pixels;
		return 0;
	}
	else {
		return pixels;
	}
}

void Image::Internal::upload(unsigned char *pixels)
{
	// To get a complete palette..
	if (dumping_colours) {
		unsigned char *rgb = pixels;
		for (int i = 0; i < size.w*size.h; i++) {
			if (rgb[3] != 0) {
				printf("rgb: %d %d %d\n", rgb[0], rgb[1], rgb[2]);
			}
			rgb += 4;
		}
	}

	if (texture == 0) {
		glGenTextures_ptr(1, &texture);
		PRINT_GL_ERROR("glGenTextures\n");
		if (texture == 0) {
			throw util::GLError("glGenTextures failed");
		}

		glBindTexture_ptr(GL_TEXTURE_2D, texture);
		PRINT_GL_ERROR("glBindTexture\n");

#if 0
		glTexImage2D_ptr(GL_TEXTURE_2D, 0, GL_RGBA4, size.w, size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
		glTexImage2D_ptr(GL_TEXTURE_2D, 0, GL_RGBA, size.w, size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif
		PRINT_GL_ERROR("glTexImage2D\n");

		if (shim::linear_filtering) {
			glGenerateMipmap_ptr(GL_TEXTURE_2D);
		}

		glTextureParameteri_ptr(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		PRINT_GL_ERROR("glTextureParameteri\n");
		glTextureParameteri_ptr(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		PRINT_GL_ERROR("glTextureParameteri\n");
		glTexParameteri_ptr(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, shim::linear_filtering ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
		PRINT_GL_ERROR("glTextureParameteri\n");
		glTexParameteri_ptr(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, shim::linear_filtering ? GL_LINEAR : GL_NEAREST);
		PRINT_GL_ERROR("glTextureParameteri\n");

		if (has_render_to_texture) {
			// Create an FBO for render-to-texture

			glGenFramebuffers_ptr(1, &fbo);
			PRINT_GL_ERROR("glGenFramebuffers\n");

			glBindFramebuffer_ptr(GL_FRAMEBUFFER, fbo);
			PRINT_GL_ERROR("glBindFramebuffer\n");

			glFramebufferTexture2D_ptr(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
			PRINT_GL_ERROR("glFramebufferTexture2D\n");

			if (this->create_depth_buffer) {
				GLenum format;
				if (this->create_stencil_buffer) {
					format = GL_DEPTH24_STENCIL8;
				}
				else {
					format = GL_DEPTH_COMPONENT16;
				}
			}

			GLenum result = glCheckFramebufferStatus_ptr(GL_FRAMEBUFFER);
			if (result != GL_FRAMEBUFFER_COMPLETE) {
				throw util::GLError("Incomplete framebuffer!");
			}
			PRINT_GL_ERROR("glCheckFramebufferStatus\n");

			glBindFramebuffer_ptr(GL_FRAMEBUFFER, bound_fbo);

			PRINT_GL_ERROR("glBindFramebuffer\n");
		}

		Shader::rebind_opengl_texture0();
	}
}

void Image::Internal::unbind()
{
	std::vector< std::pair<std::string, Image *> > &bound_images = Shader::get_bound_images();
	for (size_t unit = 0; unit < bound_images.size(); unit++) {
		std::pair<std::string, Image *> p = bound_images[unit];
		if (p.second->internal == this) {
			shim::current_shader->set_texture(p.first, 0, (int)unit);
			bound_images[unit].second = nullptr;
			break;
		}
	}
}

void Image::Internal::destroy_data()
{
	delete[] loaded_data;
	loaded_data = 0;
}

// FIXME: needs to support D3D/OpenGL upsidedown/rightsideup textures
bool Image::Internal::is_transparent(util::Point<int> position)
{
	if (loaded_data == 0) {
		return false;
	}

	unsigned char *p = loaded_data + ((((size.h-1)-position.y) * size.w) + position.x) * 4;

	return p[3] == 0;
}

} // End namespace gfx

} // End namespace noo
