#pragma once

#include <array>
#include "../../Common/Tools/Tools.h"

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

            std::unique_ptr<char> Serialize() const;
            size_t Deserialize(const char* data);
    };
}