#pragma once

#include "Maths.h"
#include "Plane.h"

namespace Engine
{
    class Frustum
    {
        
        public :

            Vector3 const& position;
            Matrix4x4 const& rotation;

            Vector4 far_left_top_point;
            Vector4 near_right_bottom_point;
            float vertical_angle;
            float horizontal_angle;
            
            Frustum(Vector3 const& position, Matrix4x4 const& rotation) : position(position), rotation(rotation) {}
            void Setup(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip);

            // Le point se trouve dans le frustum
            bool IsInside(Vector3 const& point) const;

        private :

            #pragma push_macro("near")
            #pragma push_macro("far")
            #undef near
            #undef far
            Vector3 left;
            Vector3 right;
            Vector3 top;
            Vector3 bottom;
            Vector3 near;
            Vector3 far;
            #pragma pop_macro("far")
            #pragma pop_macro("near")

            /*Vector3 const& position;
            Matrix4x4 const& rotation;

            Vector4 far_left_top_point;
            Vector4 near_right_bottom_point;
            float vertical_angle;
            float horizontal_angle;*/
    };
}