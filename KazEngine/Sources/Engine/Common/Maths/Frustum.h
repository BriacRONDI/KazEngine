#pragma once

#include "Maths.h"
#include "Plane.h"

namespace Engine
{
    class Frustum
    {
        
        public :
            
            void InitFrustum(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip);
            void MoveFrustum(Vector3 const& position, Engine::Matrix4x4 const& rotation);

            // Le point se trouve dans le frustum
            // bool IsInside(Vector3 const& point);

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

            Vector4 far_left_top_point;
            Vector4 near_right_bottom_point;
            float field_of_view;
    };
}