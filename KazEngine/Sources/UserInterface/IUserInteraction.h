#pragma once

#include <cstdint>
#include <Types.hpp>

namespace Engine
{
    class IUserInteraction
    {
        public :
            virtual void SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end) = 0;
            virtual void ToggleSelection(Point<uint32_t> mouse_position) = 0;
    };
}