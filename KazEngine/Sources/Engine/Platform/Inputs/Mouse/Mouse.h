#pragma once

#include <array>

#include "../../../Common/EventEmitter.hpp"
#include "IMouseListener.h"

namespace Engine
{
    class Mouse : public EventEmitter<IMouseListener>
    {
        public:
            
            struct MOUSE_POSITION {
                uint32_t x;
                uint32_t y;
            };

            static Mouse& GetInstance();

            void MouseMove(unsigned int x, unsigned int y);
            void MouseButtonDown(IMouseListener::MOUSE_BUTTON button);
            void MouseButtonUp(IMouseListener::MOUSE_BUTTON button);
            void MouseScrollUp();
            void MouseScrollDown();
            MOUSE_POSITION const& GetPosition();
            bool IsButtonPressed(IMouseListener::MOUSE_BUTTON button);

        private:

            MOUSE_POSITION position;
            std::array<bool, 3> button_state;
            static Mouse* Instance;

            Mouse();
            ~Mouse();
    };
}
