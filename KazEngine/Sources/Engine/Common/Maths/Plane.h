#pragma once

#include "Maths.h"

namespace Engine
{

    class Plane
    {
        public :

            Vector3 normal;
            Vector3 point;
            
            inline float SignedDistance(Vector3 point) { return this->normal.Dot(point - this->point); }
    };
}