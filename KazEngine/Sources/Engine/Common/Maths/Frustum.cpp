#include "Frustum.h"

namespace Engine
{
    void Frustum::Setup(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip)
    {
        this->vertical_angle = field_of_view * 0.5f * DEGREES_TO_RADIANS;
        this->near_plane_distance = near_clip;

        float half_tangent = std::tan(this->vertical_angle);
        this->near_plane_size.Height = half_tangent * near_clip;
        this->near_plane_size.Width = this->near_plane_size.Height * aspect_ratio;
        float far_height = half_tangent * far_clip;
	    float far_width = far_height * aspect_ratio;

        this->far_left_top_point = {-far_width, -far_height, -far_clip, 1.0f};
        this->near_right_bottom_point = {this->near_plane_size.Width, this->near_plane_size.Height, -near_clip, 1.0f};

        this->horizontal_angle = std::atan(half_tangent * aspect_ratio);
    }

    bool Frustum::IsInside(Vector3 const& point) const
    {
        Matrix4x4 rotation = this->rotation.Transpose();

        Engine::Vector3 point_flt = (Engine::Matrix4x4::TranslationMatrix(-this->position) * rotation * this->far_left_top_point).ToVector3();
        Engine::Vector3 point_nrb = (Engine::Matrix4x4::TranslationMatrix(-this->position) * rotation * this->near_right_bottom_point).ToVector3();

        Engine::Vector3 normal_left = (rotation * Engine::Matrix4x4::RotationMatrix(-this->horizontal_angle, {0.0f, 1.0f, 0.0f}, false, false) * Engine::Vector4({1.0f, 0.0f, 0.0f, 1.0f})).ToVector3();
        Engine::Vector3 normal_right = (rotation * Engine::Matrix4x4::RotationMatrix(this->horizontal_angle, {0.0f, 1.0f, 0.0f}, false, false) * Engine::Vector4({-1.0f, 0.0f, 0.0f, 1.0f})).ToVector3();
        Engine::Vector3 normal_top = (rotation * Engine::Matrix4x4::RotationMatrix(this->vertical_angle, {1.0f, 0.0f, 0.0f}, false, false) * Engine::Vector4({0.0f, 1.0f, 0.0f, 1.0f})).ToVector3();
        Engine::Vector3 normal_bottom = (rotation * Engine::Matrix4x4::RotationMatrix(-this->vertical_angle, {1.0f, 0.0f, 0.0f}, false, false) * Engine::Vector4({0.0f, -1.0f, 0.0f, 1.0f})).ToVector3();

        Engine::Vector3 normal_far = (rotation * Engine::Vector4({0.0f, 0.0f, 1.0f, 1.0f})).ToVector3();
        Engine::Vector3 normal_near = normal_far * -1;

        float dist_left = normal_left.Dot(point - point_flt);
        float dist_right = normal_right.Dot(point - point_nrb);
        float dist_top = normal_top.Dot(point - point_flt);
        float dist_bottom = normal_bottom.Dot(point - point_nrb);
        float dist_near = normal_near.Dot(point - point_nrb);
        float dist_far = normal_far.Dot(point - point_flt);

        return dist_left >= 0
               && dist_right >= 0
               && dist_top >= 0
               && dist_bottom >= 0
               && dist_near >= 0
               && dist_far >= 0;
    }
} 