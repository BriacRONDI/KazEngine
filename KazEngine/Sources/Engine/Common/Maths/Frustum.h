#pragma once

#include "Maths.h"
#include "Plane.h"
#include "../../Common/Types.hpp"

namespace Engine
{
    class Frustum
    {
        
        public :

            Frustum(Vector3 const& position, Matrix4x4 const& rotation) : position(position), rotation(rotation) {}
            void Setup(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip);
            inline Area<float> const& GetNearPlaneSize() const { return this->near_plane_size; }

            // Le point se trouve dans le frustum
            bool IsInside(Vector3 const& point) const;

        private :

            Area<float> near_plane_size;
            float near_plane_distance;
            Vector3 const& position;
            Matrix4x4 const& rotation;

            Vector4 far_left_top_point;
            Vector4 near_right_bottom_point;
            float vertical_angle;
            float horizontal_angle;
    };
}