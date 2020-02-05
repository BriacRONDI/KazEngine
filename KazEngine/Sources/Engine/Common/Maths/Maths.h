#pragma once

#define DEGREES_TO_RADIANS 0.01745329251994329576923690768489f
#define IDENTITY_MATRIX {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}
#define ZERO_MATRIX {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

#include <array>

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"

namespace Engine
{
    namespace Maths
    {
        inline float Interpolate(float source, float dest, float ratio)
        {
            if(source <= dest) return source + std::abs(dest - source) * ratio;
            else return source - std::abs(dest - source) * ratio;
        }
    }
}