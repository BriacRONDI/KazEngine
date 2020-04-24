#pragma once

#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <EventEmitter.hpp>
#include "IKeyboardListener.h"

namespace Engine
{
    class Keyboard : public Tools::EventEmitter<IKeyboardListener>
    {
        public:
            static inline Keyboard& GetInstance() { return Keyboard::instance; }
            bool IsPressed(const unsigned char Key);
            void PressKey(const unsigned char Key);
            void ReleaseKey(const unsigned char Key);

        private:
            char KeyPressed[256];
            static Keyboard instance;
            Keyboard();
            ~Keyboard();
    };
}

