#pragma once

#include "../Platform/Common/Mouse/Mouse.h"

namespace Engine
{
    class Interaction : public IMouseListener
    {
        public :
            
            //////////////////////////////
            // IMouseListener interface //
            //////////////////////////////

            virtual void MouseMove(unsigned int x, unsigned int y) {};
            virtual void MouseButtonDown(MOUSE_BUTTON button) {};
            virtual void MouseButtonUp(MOUSE_BUTTON button) {};
            virtual void MouseWheelUp() {};
            virtual void MouseWheelDown() {};

        private :

            
    };
}