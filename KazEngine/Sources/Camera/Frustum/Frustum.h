#pragma once

#include <Types.hpp>
#include <Maths.h>

namespace Engine
{
    class Frustum
    {
        
        public :

            enum PLANE_SIDE {
                LEFT    = 0,
                RIGHT   = 1,
                TOP     = 2,
                BOTTOM  = 3,
                BACK    = 4,
                FRONT   = 5
            };

            Frustum(Maths::Vector3 const& position, Maths::Matrix4x4 const& rotation) : position(position), rotation(rotation) {}
            void Setup(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip);
            inline Area<float> const& GetNearPlaneSize() const { return this->near_plane_size; }
            void UpdatePlanes(Maths::Matrix4x4 matrix);
            inline std::array<Maths::Vector4, 6>& GetPlanes() { return this->planes; }

        private :

            Area<float> near_plane_size;
            float near_plane_distance;
            Maths::Vector3 const& position;
            Maths::Matrix4x4 const& rotation;
            Maths::Vector4 far_left_top_point;
            Maths::Vector4 near_right_bottom_point;
            float vertical_angle;
            float horizontal_angle;
            std::array<Maths::Vector4, 6> planes;
    };
}