#pragma once

#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include "IKeyboardListener.h"
#include "../../../Common/EventEmitter.hpp"

namespace Engine
{
    class Keyboard : public EventEmitter<IKeyboardListener>
    {
        public:
            static Keyboard* GetInstance();
            bool IsPressed(const unsigned char Key);
            void PressKey(const unsigned char Key);
            void ReleaseKey(const unsigned char Key);

        private:
            char KeyPressed[256];
            static Keyboard* Instance;
            Keyboard();
            ~Keyboard();
    };
}

