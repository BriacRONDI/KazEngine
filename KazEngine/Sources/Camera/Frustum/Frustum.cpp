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

    void Frustum::UpdatePlanes(Maths::Matrix4x4 matrix)
    {
        this->planes[LEFT].x = matrix[3] + matrix[0];
		this->planes[LEFT].y = matrix[7] + matrix[4];
		this->planes[LEFT].z = matrix[11] + matrix[8];
		this->planes[LEFT].w = matrix[15] + matrix[12];

		this->planes[RIGHT].x = matrix[3] - matrix[0];
		this->planes[RIGHT].y = matrix[7] - matrix[4];
		this->planes[RIGHT].z = matrix[11] - matrix[8];
		this->planes[RIGHT].w = matrix[15] - matrix[12];

		this->planes[TOP].x = matrix[3] - matrix[1];
		this->planes[TOP].y = matrix[7] - matrix[5];
		this->planes[TOP].z = matrix[11] - matrix[9];
		this->planes[TOP].w = matrix[15] - matrix[13];

		this->planes[BOTTOM].x = matrix[3] + matrix[1];
		this->planes[BOTTOM].y = matrix[7] + matrix[5];
		this->planes[BOTTOM].z = matrix[11] + matrix[9];
		this->planes[BOTTOM].w = matrix[15] + matrix[13];

		this->planes[BACK].x = matrix[3] + matrix[2];
		this->planes[BACK].y = matrix[7] + matrix[6];
		this->planes[BACK].z = matrix[11] + matrix[10];
		this->planes[BACK].w = matrix[15] + matrix[14];

		this->planes[FRONT].x = matrix[3] - matrix[2];
		this->planes[FRONT].y = matrix[7] - matrix[6];
		this->planes[FRONT].z = matrix[11] - matrix[10];
		this->planes[FRONT].w = matrix[15] - matrix[14];

		for(auto i=0; i<planes.size(); i++)
		{
			float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
			planes[i] /= length;
		}
    }
} 