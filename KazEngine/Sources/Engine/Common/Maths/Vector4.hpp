#pragma once

#include <array>

namespace Engine
{
    class Vector4
    {
        public :

            union
            {
                struct 
                {
                    float x;
                    float y;
                    float z;
                    float w;
                };
                std::array<float, 4> xyzw;
            };
    };
}