#include "Frustum.h"

namespace Engine
{
    void Frustum::InitFrustum(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip)
    {
        this->field_of_view = field_of_view;

        float half_tangent = std::tan(field_of_view / 2);
        float near_height = half_tangent * near_clip;
        float near_width = near_height * aspect_ratio;
        float far_height = half_tangent * far_clip;
	    float far_width = far_height * aspect_ratio;

        this->far_left_top_point = {-far_width, -far_height, -far_clip, 1.0f};
        this->near_right_bottom_point = {near_width, near_height, near_clip, 1.0f};
    }

    void Frustum::MoveFrustum(Vector3 const& position, Engine::Matrix4x4 const& rotation)
    {
        Engine::Vector4 point_flt = Engine::Matrix4x4::TranslationMatrix(position) * rotation * this->far_left_top_point;
        Engine::Vector4 point_nrb = Engine::Matrix4x4::TranslationMatrix(position) * rotation * this->near_right_bottom_point;
    }
}