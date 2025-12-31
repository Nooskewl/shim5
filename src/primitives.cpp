#include "shim5/cd.h"
#include "shim5/primitives.h"
#include "shim5/shim.h"
#include "shim5/util.h"
#include "shim5/vertex_cache.h"

using namespace noo;

static bool primitives_held;
static bool vertex_cache_already_held;

static void draw_line_worker(SDL_Color colour, noo::util::Point<float> a, noo::util::Point<float> b, float thickness)
{
	float half_thickness = thickness / 2.0f;
	SDL_Color vertex_colours[4];
	for (int i = 0; i < 4; i++) {
		vertex_colours[i] = colour;
	}
	float dx = float(a.x - b.x);
	float dy = float(a.y - b.y);
	float angle = atan2f(dy, dx);
	/* Make 4 points for thickness */
	float a1 = angle + (float)M_PI / 2.0f;
	float a2 = angle - (float)M_PI / 2.0f;
	noo::util::Point<float> da = a;
	noo::util::Point<float> db = a;
	noo::util::Point<float> dc = b;
	noo::util::Point<float> dd = b;
	da.x += cos(a1) * half_thickness;
	da.y += sin(a1) * half_thickness;
	db.x += cos(a2) * half_thickness;
	db.y += sin(a2) * half_thickness;
	dc.x += cos(a1) * half_thickness;
	dc.y += sin(a1) * half_thickness;
	dd.x += cos(a2) * half_thickness;
	dd.y += sin(a2) * half_thickness;
	noo::gfx::Vertex_Cache::instance()->cache(vertex_colours, {0.0f, 0.0f}, {0.0f, 0.0f}, da, dc, dd, db, 0);
}

static void draw_straight_line_worker(SDL_Color colour, noo::util::Point<float> a, noo::util::Point<float> b, float thickness)
{
	SDL_Color vertex_colours[4];
	for (int i = 0; i < 4; i++) {
		vertex_colours[i] = colour;
	}
	noo::util::Point<float> da, db, dc, dd;
	if (a.x == b.x) {
		da = a;
		db = a+noo::util::Point<float>(thickness, 0.0f);
		dc = b+noo::util::Point<float>(thickness, 0.0f);
		dd = b;
	}
	else {
		da = a;
		db = b;
		dc = b+noo::util::Point<float>(0.0f, thickness);
		dd = a+noo::util::Point<float>(0.0f, thickness);
	}
	noo::gfx::Vertex_Cache::instance()->cache(vertex_colours, {0.0f, 0.0f}, {0.0f, 0.0f}, da, db, dc, dd, 0);
}

namespace noo {

namespace gfx {

void static_start_primitives()
{
	primitives_held = false;
	vertex_cache_already_held = false;
}

void draw_primitives_start()
{
	if (noo::gfx::Vertex_Cache::instance()->is_started()) {
		vertex_cache_already_held = true;
	}
	else {
		noo::gfx::Vertex_Cache::instance()->start();
	}

	primitives_held = true;
}

void draw_primitives_end()
{
	if (vertex_cache_already_held == false) {
		noo::gfx::Vertex_Cache::instance()->end();
	}
	else {
		vertex_cache_already_held = false;
	}
	primitives_held = false;
}

void draw_line(SDL_Color colour, util::Point<float> a, util::Point<float> b, float thickness)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	draw_line_worker(colour, a, b, thickness);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_triangle_3d(SDL_Color vertex_colours[3], util::Vec3D<float> a, util::Vec3D<float> b, util::Vec3D<float> c)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}

	static float verts[9];
	static int faces[3];
	static float colours[12];

	verts[0] = a.x;
	verts[1] = a.y;
	verts[2] = a.z;
	verts[3] = a.x;
	verts[4] = a.y;
	verts[5] = a.z;
	verts[6] = a.x;
	verts[7] = a.y;
	verts[8] = a.z;

	faces[0] = 0;
	faces[1] = 0;
	faces[2] = 0;

	colours[0] = vertex_colours[0].r / 255.0f;
	colours[1] = vertex_colours[0].g / 255.0f;
	colours[2] = vertex_colours[0].b / 255.0f;
	colours[3] = vertex_colours[0].a / 255.0f;
	colours[4] = vertex_colours[1].r / 255.0f;
	colours[5] = vertex_colours[1].g / 255.0f;
	colours[6] = vertex_colours[1].b / 255.0f;
	colours[7] = vertex_colours[1].a / 255.0f;
	colours[8] = vertex_colours[2].r / 255.0f;
	colours[9] = vertex_colours[2].g / 255.0f;
	colours[10] = vertex_colours[2].b / 255.0f;
	colours[11] = vertex_colours[2].a / 255.0f;

	Vertex_Cache::instance()->cache_3d(shim::white, verts, faces, nullptr, nullptr, colours, 1);

	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_triangle_3d(SDL_Color colour, util::Vec3D<float> a, util::Vec3D<float> b, util::Vec3D<float> c)
{
	SDL_Color vertex_colours[3];

	vertex_colours[0] = colour;
	vertex_colours[1] = colour;
	vertex_colours[2] = colour;

	draw_triangle_3d(vertex_colours, a, b, c);
}

void draw_filled_triangle(SDL_Color vertex_colours[3], util::Point<float> a, util::Point<float> b, util::Point<float> c)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	// make sure winding order is right, ALWAYS draw triangles with this func
	if ((b.x*a.y+c.x*b.y+a.x*c.y) > (a.x*b.y+b.x*c.y+c.x*a.y)) {
		util::Point<float> tmp;
		tmp = c;
		c = b;
		b = tmp;
		SDL_Color tmpc;
		tmpc = vertex_colours[2];
		vertex_colours[2] = vertex_colours[1];
		vertex_colours[1] = tmpc;
	}
	Vertex_Cache::instance()->cache(vertex_colours, a, b, c);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_triangle(SDL_Color colour, util::Point<float> a, util::Point<float> b, util::Point<float> c)
{
	static SDL_Color vertex_colours[3];
	vertex_colours[0] = colour;
	vertex_colours[1] = colour;
	vertex_colours[2] = colour;
	draw_filled_triangle(vertex_colours, a, b, c);
}

static bool get_pts(double thick, double x1, double y1, double x2, double y2, double x3, double y3, double xx1, double yy1, double xx2, double yy2, double xx3, double yy3, double xx4, double yy4, double *outx1, double *outy1, double *outx2, double *outy2)
{
	double xxx1, yyy1, xxx2, yyy2;

	xxx1 = (xx1 + xx2) / 2;
	yyy1 = (yy1 + yy2) / 2;
	xxx2 = (xx3 + xx4) / 2;
	yyy2 = (yy3 + yy4) / 2;

	double a1, a2;

	a1 = atan2(yy2-yy1, xx2-xx1);
	a2 = atan2(yy4-yy3, xx4-xx3);

	double aa11, aa12, aa21, aa22;

	aa11 = a1 + (M_PI/2.0);
	aa12 = a1 - (M_PI/2.0);
	aa21 = a2 - (M_PI/2.0);
	aa22 = a2 + (M_PI/2.0);

	double _x1, _y1, _x2, _y2, _x3, _y3, _x4, _y4;

	_x1 = xxx1 + cos(aa11) * (thick / 2.0);
	_y1 = yyy1 + sin(aa11) * (thick / 2.0);
	_x2 = xxx1 + cos(aa12) * (thick / 2.0);
	_y2 = yyy1 + sin(aa12) * (thick / 2.0);
	_x3 = xxx2 + cos(aa21) * (thick / 2.0);
	_y3 = yyy2 + sin(aa21) * (thick / 2.0);
	_x4 = xxx2 + cos(aa22) * (thick / 2.0);
	_y4 = yyy2 + sin(aa22) * (thick / 2.0);

	double __x1, __y1, __x2, __y2, __x3, __y3, __x4, __y4;

	const double BIGNUM = 1000000000.0;

	__x1 = _x1 + cos(a1) * BIGNUM;
	__y1 = _y1 + sin(a1) * BIGNUM;
	__x2 = _x2 + cos(a1) * BIGNUM;
	__y2 = _y2 + sin(a1) * BIGNUM;
	__x3 = _x3 + cos(a2) * BIGNUM;
	__y3 = _y3 + sin(a2) * BIGNUM;
	__x4 = _x4 + cos(a2) * BIGNUM;
	__y4 = _y4 + sin(a2) * BIGNUM;

	util::Point<float> p1, p2, p3, p4;

	p1.x = _x1;
	p1.y = _y1;
	p2.x = __x1;
	p2.y = __y1;
	p3.x = _x3;
	p3.y = _y3;
	p4.x = __x3;
	p4.y = __y3;

	util::Point<float> result;

	if (cd::line_line(&p1, &p2, &p3, &p4, &result) == false) {
		return false;
	}

	*outx1 = result.x;
	*outy1 = result.y;

	p1.x = _x2;
	p1.y = _y2;
	p2.x = __x2;
	p2.y = __y2;
	p3.x = _x4;
	p3.y = _y4;
	p4.x = __x4;
	p4.y = __y4;

	if (cd::line_line(&p1, &p2, &p3, &p4, &result) == false) {
		return false;
	}

	*outx2 = result.x;
	*outy2 = result.y;

	return true;
}

void draw_triangle(SDL_Color colour, util::Point<float> a, util::Point<float> b, util::Point<float> c, float thickness)
{
	double x1 = a.x;
	double y1 = a.y;
	double x2 = b.x;
	double y2 = b.y;
	double x3 = c.x;
	double y3 = c.y;

	double results[6][2];
	double xx1, yy1, xx2, yy2, xx3, yy3, xx4, yy4;

	xx1 = x1;
	yy1 = y1;
	xx2 = x2;
	yy2 = y2;
	xx3 = x3;
	yy3 = y3;
	xx4 = x2;
	yy4 = y2;

	if (get_pts(thickness, x1, y1, x2, y2, x3, y3, xx1, yy1, xx2, yy2, xx3, yy3, xx4, yy4, &results[0][0], &results[0][1], &results[1][0], &results[1][1]) == false) {
		double xx1, yy1, xx2, yy2, xx3, yy3;
		double _x, _y;
		double dx, dy;
		float a;
		_x = (x2 + x3) / 2.0;
		_y = (y2 + y3) / 2.0;
		dx = x1 - _x;
		dy = y1 - _y;
		a = atan2(dy, dx);
		xx1 = x1 + cos(a) * (thickness / 2.0);
		yy1 = y1 + sin(a) * (thickness / 2.0);
		_x = (x1 + x3) / 2.0;
		_y = (y1 + y3) / 2.0;
		dx = x2 - _x;
		dy = y2 - _y;
		a = atan2(dy, dx);
		xx2 = x2 + cos(a) * (thickness / 2.0);
		yy2 = y2 + sin(a) * (thickness / 2.0);
		_x = (x1 + x2) / 2.0;
		_y = (y1 + y2) / 2.0;
		dx = x3 - _x;
		dy = y3 - _y;
		a = atan2(dy, dx);
		xx3 = x3 + cos(a) * (thickness / 2.0);
		yy3 = y3 + sin(a) * (thickness / 2.0);
		gfx::draw_filled_triangle(colour, util::Point<float>(xx1, yy1), util::Point<float>(xx2, yy2), util::Point<float>(xx3, yy3));
		return;
	}

	xx1 = x2;
	yy1 = y2;
	xx2 = x3;
	yy2 = y3;
	xx3 = x1;
	yy3 = y1;
	xx4 = x3;
	yy4 = y3;

	if (get_pts(thickness, x1, y1, x2, y2, x3, y3, xx1, yy1, xx2, yy2, xx3, yy3, xx4, yy4, &results[2][0], &results[2][1], &results[3][0], &results[3][1]) == false) {
		double xx1, yy1, xx2, yy2, xx3, yy3;
		double _x, _y;
		double dx, dy;
		float a;
		_x = (x2 + x3) / 2.0;
		_y = (y2 + y3) / 2.0;
		dx = x1 - _x;
		dy = y1 - _y;
		a = atan2(dy, dx);
		xx1 = x1 + cos(a) * (thickness / 2.0);
		yy1 = y1 + sin(a) * (thickness / 2.0);
		_x = (x1 + x3) / 2.0;
		_y = (y1 + y3) / 2.0;
		dx = x2 - _x;
		dy = y2 - _y;
		a = atan2(dy, dx);
		xx2 = x2 + cos(a) * (thickness / 2.0);
		yy2 = y2 + sin(a) * (thickness / 2.0);
		_x = (x1 + x2) / 2.0;
		_y = (y1 + y2) / 2.0;
		dx = x3 - _x;
		dy = y3 - _y;
		a = atan2(dy, dx);
		xx3 = x3 + cos(a) * (thickness / 2.0);
		yy3 = y3 + sin(a) * (thickness / 2.0);
		gfx::draw_filled_triangle(colour, util::Point<float>(xx1, yy1), util::Point<float>(xx2, yy2), util::Point<float>(xx3, yy3));
		return;
	}

	xx1 = x3;
	yy1 = y3;
	xx2 = x1;
	yy2 = y1;
	xx3 = x2;
	yy3 = y2;
	xx4 = x1;
	yy4 = y1;

	if (get_pts(thickness, x1, y1, x2, y2, x3, y3, xx1, yy1, xx2, yy2, xx3, yy3, xx4, yy4, &results[4][0], &results[4][1], &results[5][0], &results[5][1]) == false) {
		double xx1, yy1, xx2, yy2, xx3, yy3;
		double _x, _y;
		double dx, dy;
		float a;
		_x = (x2 + x3) / 2.0;
		_y = (y2 + y3) / 2.0;
		dx = x1 - _x;
		dy = y1 - _y;
		a = atan2(dy, dx);
		xx1 = x1 + cos(a) * (thickness / 2.0);
		yy1 = y1 + sin(a) * (thickness / 2.0);
		_x = (x1 + x3) / 2.0;
		_y = (y1 + y3) / 2.0;
		dx = x2 - _x;
		dy = y2 - _y;
		a = atan2(dy, dx);
		xx2 = x2 + cos(a) * (thickness / 2.0);
		yy2 = y2 + sin(a) * (thickness / 2.0);
		_x = (x1 + x2) / 2.0;
		_y = (y1 + y2) / 2.0;
		dx = x3 - _x;
		dy = y3 - _y;
		a = atan2(dy, dx);
		xx3 = x3 + cos(a) * (thickness / 2.0);
		yy3 = y3 + sin(a) * (thickness / 2.0);
		gfx::draw_filled_triangle(colour, util::Point<float>(xx1, yy1), util::Point<float>(xx2, yy2), util::Point<float>(xx3, yy3));
		return;
	}
	
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	
	gfx::draw_filled_triangle(colour, util::Point<float>(x2, y2), util::Point<float>(results[0][0], results[0][1]), util::Point<float>(x1, y1));
	gfx::draw_filled_triangle(colour, util::Point<float>(x1, y1), util::Point<float>(results[4][0], results[4][1]), util::Point<float>(results[0][0], results[0][1]));
	gfx::draw_filled_triangle(colour, util::Point<float>(x2, y2), util::Point<float>(results[1][0], results[1][1]), util::Point<float>(x1, y1));
	gfx::draw_filled_triangle(colour, util::Point<float>(x1, y1), util::Point<float>(results[5][0], results[5][1]), util::Point<float>(results[1][0], results[1][1]));


	gfx::draw_filled_triangle(colour, util::Point<float>(x3, y3), util::Point<float>(results[2][0], results[2][1]), util::Point<float>(x1, y1));
	gfx::draw_filled_triangle(colour, util::Point<float>(x1, y1), util::Point<float>(results[4][0], results[4][1]), util::Point<float>(results[2][0], results[2][1]));
	gfx::draw_filled_triangle(colour, util::Point<float>(x3, y3), util::Point<float>(results[3][0], results[3][1]), util::Point<float>(x1, y1));
	gfx::draw_filled_triangle(colour, util::Point<float>(x1, y1), util::Point<float>(results[5][0], results[5][1]), util::Point<float>(results[3][0], results[3][1]));


	gfx::draw_filled_triangle(colour, util::Point<float>(x3, y3), util::Point<float>(results[2][0], results[2][1]), util::Point<float>(x2, y2));
	gfx::draw_filled_triangle(colour, util::Point<float>(x2, y2), util::Point<float>(results[0][0], results[0][1]), util::Point<float>(results[2][0], results[2][1]));
	gfx::draw_filled_triangle(colour, util::Point<float>(x3, y3), util::Point<float>(results[3][0], results[3][1]), util::Point<float>(x2, y2));
	gfx::draw_filled_triangle(colour, util::Point<float>(x2, y2), util::Point<float>(results[1][0], results[1][1]), util::Point<float>(results[3][0], results[3][1]));
	
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_rectangle(SDL_Color colour, util::Point<float> pos, util::Size<float> size, float thickness)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	// top
	draw_straight_line_worker(colour, pos, pos+util::Size<float>(size.w, 0.0f), thickness);
	// bottom
	draw_straight_line_worker(colour, pos+util::Size<float>(0.0f, size.h-thickness), pos+util::Size<float>(size.w, size.h-thickness), thickness);
	// left
	draw_straight_line_worker(colour, pos+util::Point<float>(0.0f, thickness), pos+util::Size<float>(0.0f, size.h-thickness), thickness);
	// right
	draw_straight_line_worker(colour, pos+util::Point<float>(size.w-thickness, thickness), pos+util::Size<float>(size.w-thickness, size.h-thickness), thickness);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_ellipse(SDL_Color colour, util::Point<float> centre, float rx, float ry, int sections, float start_angle)
{
	SDL_Color colours[3] = { colour, colour, colour };

	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}

	if (sections == -1) {
		sections = MAX(rx, ry) * 2.0f;
		// More than this is unnecessary and slow
		if (sections > 4096) {
			sections = 4096;
		}
	}

	// The algorithm won't work with less than 4
	if (sections < 4) {
		sections = 4;
	}

	for (int n = 0; n < sections; n++) {
		int n2 = (n+1) % sections;
		float a1 = start_angle + (float)n/sections * (float)M_PI * 2.0f;
		float a2 = start_angle + (float)n2/sections * (float)M_PI * 2.0f;
		noo::util::Point<float> a, b;
		a = centre + noo::util::Point<float>(cos(a1) * rx, sin(a1) * ry);
		b = centre + noo::util::Point<float>(cos(a2) * rx, sin(a2) * ry);
		noo::gfx::Vertex_Cache::instance()->cache(colours, centre, a, b);
	}

	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_ellipse(SDL_Color colour, util::Point<float> centre, float rx, float ry, float thickness, int sections, float start_angle)
{
	SDL_Color colours[3] = { colour, colour, colour };

	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}

	if (sections == -1) {
		sections = MAX(rx, ry) * 2.0f;
		// More than this is unnecessary and slow
		if (sections > 4096) {
			sections = 4096;
		}
	}

	// The algorithm won't work with less than 4
	if (sections < 4) {
		sections = 4;
	}

	for (int n = 0; n < sections; n++) {
		int n2 = (n+1) % sections;
		float a1 = start_angle + (float)n/sections * (float)M_PI * 2.0f;
		float a2 = start_angle + (float)n2/sections * (float)M_PI * 2.0f;
		float half = 0.5f/sections * (float)M_PI * 2.0f;
		//float a3 = a1 + half;
		//float a4 = (n == sections - 1) ? half : a2 + half;
		float a3 = a1;
		float a4 = a2;
		noo::util::Point<float> a, b, c, d;
		a = centre + noo::util::Point<float>(cos(a1) * rx, sin(a1) * ry);
		b = centre + noo::util::Point<float>(cos(a2) * rx, sin(a2) * ry);
		c = centre + noo::util::Point<float>(cos(a3) * (rx - thickness), sin(a3) * (ry - thickness));
		d = centre + noo::util::Point<float>(cos(a4) * (rx - thickness), sin(a4) * (ry - thickness));
		noo::gfx::Vertex_Cache::instance()->cache(colours, a, b, c);
		noo::gfx::Vertex_Cache::instance()->cache(colours, b, d, c);
	}

	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_circle(SDL_Color colour, util::Point<float> centre, float radius, int sections, float start_angle)
{
	draw_filled_ellipse(colour, centre, radius, radius, sections, start_angle);
}

void draw_circle(SDL_Color colour, util::Point<float> centre, float radius, float thickness, int sections, float start_angle)
{
	draw_ellipse(colour, centre, radius, radius, thickness, sections, start_angle);
}

void draw_filled_rectangle(SDL_Color vertex_colours[4], util::Point<float> dest_position, util::Size<float> dest_size)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	Vertex_Cache::instance()->cache(vertex_colours, {0.0f, 0.0f}, {0.0f, 0.0f}, dest_position, dest_size, 0);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_rectangle(SDL_Color colour, util::Point<float> dest_position, util::Size<float> dest_size)
{
	static SDL_Color vertex_colours[4];
	for (int i = 0; i < 4; i++) {
		vertex_colours[i] = colour;
	}
	draw_filled_rectangle(vertex_colours, dest_position, dest_size);
}

} // End namespace gfx

} // End namespace noo
