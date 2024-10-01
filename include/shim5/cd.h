#ifndef NOO_CD_H
#define NOO_CD_H

#include "shim5/main.h"
#include "shim5/model.h"

namespace noo {

namespace cd {

bool SHIM5_EXPORT box_box(util::Point<float> topleft_a, util::Point<float> bottomright_a, util::Point<float> topleft_b, util::Point<float> bottomright_b);
bool SHIM5_EXPORT box_box(util::Point<float> topleft_a, util::Size<float> size_a, util::Point<float> topleft_b, util::Size<float> size_b);
bool SHIM5_EXPORT line_line(const util::Point<float> *a1, const util::Point<float> *a2,	const util::Point<float> *a3, const util::Point<float> *a4, util::Point<float> *result);
float SHIM5_EXPORT dist_point_line(util::Point<float> point, util::Point<float> a, util::Point<float> b);
bool SHIM5_EXPORT model_point(gfx::Model *model, glm::mat4 transform, glm::vec3 point);
bool SHIM5_EXPORT model_line_segment(gfx::Model *model, glm::mat4 transform, glm::vec3 point1, glm::vec3 point2, glm::vec3 &out);

} // End namespace cd

} // End namespace noo

#endif // NOO_CD_H
