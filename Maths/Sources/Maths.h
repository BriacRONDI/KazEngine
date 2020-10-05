#pragma once

#define VECTOR_N 4
#include "./Vector/Vector.hpp"

#define VECTOR_N 3
#include "./Vector/Vector.hpp"

#define VECTOR_N 2
#include "./Vector/Vector.hpp"

#include "./Matrix/Matrix.hpp"
#include "./Others/Others.hpp"

namespace Maths
{
    constexpr long double L_PI = 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899L;
    constexpr long double L_PI_2 = L_PI / 2.0L;
    constexpr long double L_PI_4 = L_PI / 4.0L;
    constexpr long double L_2_PI = L_PI * 2.0L;
    constexpr long double L_SQRT_2 = 1.41421356237309504880168872420969807856967187537694807317667973799;

    constexpr float F_PI = static_cast<float>(L_PI);
    constexpr float F_PI_2 = static_cast<float>(L_PI_2);
    constexpr float F_PI_4 = static_cast<float>(L_PI_4);
    constexpr float F_2_PI = static_cast<float>(L_2_PI);
    constexpr float F_SQRT_2 = static_cast<float>(L_SQRT_2);
}