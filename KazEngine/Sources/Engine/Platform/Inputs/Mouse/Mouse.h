#pragma once

#include <array>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "../../../Common/EventEmitter.hpp"
#include "../../../Common/Types.hpp"
#include "IMouseListener.h"

namespace Engine
{
    class Mouse : public EventEmitter<IMouseListener>
    {
        public:

            static Mouse& GetInstance();

            void MouseMove(unsigned int x, unsigned int y);
            void MouseButtonDown(IMouseListener::MOUSE_BUTTON button);
            void MouseButtonUp(IMouseListener::MOUSE_BUTTON button);
            void MouseScrollUp();
            void MouseScrollDown();
            Point<uint32_t> const& GetPosition();
            bool IsButtonPressed(IMouseListener::MOUSE_BUTTON button);
            void Clip(Point<uint32_t> const& clip_origin, Area<uint32_t> const& clip_size);
            void UnClip();
            inline bool IsClipped() { return this->clipped; }

        private:

            Point<uint32_t> position;
            bool clipped;
            std::array<bool, 3> button_state;
            static Mouse* Instance;

            Mouse();
            ~Mouse();
    };
}
