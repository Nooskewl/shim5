#include "shim4/cd.h"

using namespace noo;

namespace noo {

namespace cd {

bool box_box(util::Point<float> topleft_a, util::Point<float> bottomright_a, util::Point<float> topleft_b, util::Point<float> bottomright_b)
{
	if (topleft_b.x > bottomright_a.x || bottomright_b.x < topleft_a.x || topleft_b.y > bottomright_a.y || bottomright_b.y < topleft_a.y) {
		return false;
	}
	return true;
}

bool box_box(util::Point<float> topleft_a, util::Size<float> size_a, util::Point<float> topleft_b, util::Size<float> size_b)
{
	return box_box(topleft_a, topleft_a+size_a, topleft_b, topleft_b+size_b);
}

// NOTE: code found here: http://forums.create.msdn.com/forums/t/280.aspx
bool line_line(
	const util::Point<float> *a1,
	const util::Point<float> *a2,
	const util::Point<float> *a3,
	const util::Point<float> *a4,
	util::Point<float> *result)
{
	double Ua, Ub;

	Ua = ((a4->x - a3->x) * (a1->y - a3->y) - (a4->y - a3->y) * (a1->x - a3->x)) / ((a4->y - a3->y) * (a2->x - a1->x) - (a4->x - a3->x) * (a2->y - a1->y));

	Ub = ((a2->x - a1->x) * (a1->y - a3->y) - (a2->y - a1->y) * (a1->x - a3->x)) / ((a4->y - a3->y) * (a2->x - a1->x) - (a4->x - a3->x) * (a2->y - a1->y));

	if (Ua >= 0.0f && Ua <= 1.0f && Ub >= 0.0f && Ub <= 1.0f)
	{
		if (result) {
			result->x = float(a1->x + Ua * (a2->x - a1->x));
			result->y = float(a1->y + Ua * (a2->y - a1->y));
		}

		return true;
	}
	else
	{
		return false;
	}
}

// helper function: NOTE: does not account for point INSIDE box (used for box circle detection above)
static float dist_point_box(util::Point<float> point, util::Point<float> topleft, util::Point<float> bottomright)
{
	float xdist[2], ydist[2];

	xdist[0] = point.x - topleft.x;
	xdist[1] = point.x - bottomright.x;
	ydist[0] = point.y - topleft.y;
	ydist[1] = point.y - bottomright.y;

	int minx = fabs(xdist[0]) < fabs(xdist[1]) ? 0 : 1;
	int miny = fabs(ydist[0]) < fabs(ydist[1]) ? 0 : 1;

	return sqrtf((float)(minx*minx + miny*miny));
}

static float dist_point_line_result(util::Point<float> point, util::Point<float> a, util::Point<float> b, util::Point<float> *result)
{
	// above function checks distance to end points of line first, so that's taken care of
	// and we're guaranteed there's a perpendicular line crossing the line segment

	// find distance by finding nearest intersection (perpendicular line from point to a,b)
	// find slope of line
	float run = b.y - a.y;

	// don't divide by zero
	if (run == 0) {
		return dist_point_box(point, a, b);
	}

	float rise = b.x - a.x;
	float slope = rise / run;

	float neg_slope = -slope;
	const float neg_run = 1000000; // really big number, this is the max detectable distance
	float neg_rise = neg_slope * neg_run;

	util::Point<float> c = {point.x - neg_run, point.y - neg_rise};
	util::Point<float> d = {point.x + neg_run, point.y + neg_rise};

	if (line_line(&a, &b, &c, &d, result)) {
		float dx = point.x - result->x;
		float dy = point.y - result->y;
		return sqrtf(dx*dx + dy*dy);
	}

	// Return distance to nearest endpoint
	return MIN((point-a).length(), (point-b).length());
}

float dist_point_line(util::Point<float> point, util::Point<float> a, util::Point<float> b)
{
	util::Point<float> result;
	return dist_point_line_result(point, a, b, &result);
}

static bool ray_collides(glm::vec3 ray_origin, glm::vec3 ray_vector, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 &out)
{
	constexpr float epsilon = std::numeric_limits<float>::epsilon();

	glm::vec3 edge1 = b - a;
	glm::vec3 edge2 = c - a;
	glm::vec3 ray_cross_e2 = glm::cross(ray_vector, edge2);
	float det = glm::dot(edge1, ray_cross_e2);

	if (det > -epsilon && det < epsilon) {
		return false; // This ray is parallel to this 
	}

	float inv_det = 1.0 / det;
	glm::vec3 s = ray_origin - a;
	float u = inv_det * glm::dot(s, ray_cross_e2);

	if (u < 0 || u > 1) {
		return false;
	}

	glm::vec3 s_cross_e1 = glm::cross(s, edge1);
	float v = inv_det * glm::dot(ray_vector, s_cross_e1);

	if (v < 0 || u + v > 1) {
		return false;
	}

	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = inv_det * glm::dot(edge2, s_cross_e1);

	if (t > epsilon) { // ray intersection
		out = glm::vec3(ray_origin + ray_vector * t);
		return true;
	}

	return false;
}

bool model_point(gfx::Model *model, glm::mat4 transform, glm::vec3 point)
{
	std::string anim = model->get_current_animation();
	int frame = model->get_current_frame();
	float x = point.x;
	float y = point.y;
	float z = point.z;

	gfx::Model::Node *node = model->find("Model");
	if (node == nullptr) {
		std::vector<gfx::Model::Node *> nodes = model->get_nodes();
		if (nodes.size() > 0) {
			node = nodes[0];
		}
	}

	int nt = node->num_triangles;

	float *verts = model->calc_frame(anim, frame);

	glm::vec3 ray(1.0f, 0.0f, 0.0f);

	int num_collisions = 0;

	std::vector<glm::vec3> collided;

	for (int i = 0; i < nt; i++) {
		glm::vec4 pt[3];
		for (int vert = 0; vert < 3; vert++) {
			pt[vert].x = verts[i*36+vert*12+0];
			pt[vert].y = verts[i*36+vert*12+1];
			pt[vert].z = verts[i*36+vert*12+2];
			pt[vert].w = 1.0f;
			pt[vert] = transform * pt[vert];
		}
		glm::vec3 pt3[3];
		for (int j = 0; j < 3; j++) {
			pt3[j].x = pt[j].x;
			pt3[j].y = pt[j].y;
			pt3[j].z = pt[j].z;
		}

		glm::vec3 out;
		if (ray_collides(glm::vec3(x, y, z), ray, pt3[0], pt3[1], pt3[2], out)) {
			bool already = false;
			for (size_t j = 0; j < collided.size(); j++) {
				if (collided[j] == out) {
					already = true;
					break;
				}
			}
			if (already == false) {
				collided.push_back(out);
				num_collisions++;
			}
		}
	}

	if (num_collisions % 2 == 1) {
		return true;
	}
	else {
		return false;
	}
}

} // End namespace cd

} // End namespace noo
