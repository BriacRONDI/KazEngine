#pragma once

#define VECTOR_N 3
#include "../Vector/Vector.hpp"

namespace Maths
{
    class Plane
    {
        public :
            Vector3 origin;
            Vector3 normal;
    };
}