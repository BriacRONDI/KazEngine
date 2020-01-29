#pragma once

#include <vector>
#include "../../Common/Tools/Tools.h"
#include <array>

namespace Engine
{
    class Vector2
    {
        public :
            union
            {
                struct 
                {
                    float x;
                    float y;
                };
                std::array<float, 2> xy;
            };

            std::unique_ptr<char> Serialize() const;
            size_t Deserialize(const char* data);
    };
}