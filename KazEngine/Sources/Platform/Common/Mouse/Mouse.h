#pragma once

#include <array>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <EventEmitter.hpp>
#include <Types.hpp>
#include "IMouseListener.h"

namespace Engine
{
    class Mouse : public Tools::EventEmitter<IMouseListener>
    {
        public:

            static inline Mouse& GetInstance() { return Mouse::instance; }

            void MouseMove(unsigned int x, unsigned int y);
            void MouseButtonDown(IMouseListener::MOUSE_BUTTON button);
            void MouseButtonUp(IMouseListener::MOUSE_BUTTON button);
            void MouseWheelUp();
            void MouseWheelDown();
            Point<uint32_t> const& GetPosition();
            bool IsButtonPressed(IMouseListener::MOUSE_BUTTON button);
            void Clip(Point<uint32_t> const& clip_origin, Area<uint32_t> const& clip_size);
            void UnClip();
            inline bool IsClipped() { return this->clipped; }

        private:

            Point<uint32_t> position;
            bool clipped;
            std::array<bool, 3> button_state;
            static Mouse instance;

            Mouse();
            ~Mouse();
    };
}
