#pragma once

#include <cstdint>
#include <Types.hpp>
#include "../../Common/Window/Window.h"


namespace Engine
{
    /**
     * Allow a client object to recieve window messages
     */
    class IWindowListener
    {
        public:

            enum E_WINDOW_STATE {
                WINDOW_STATE_WINDOWED   = 0,    // Fen�tre avec bords
                WINDOW_STATE_MINIMIZED  = 1,    // Fen�tre minimis�e
                WINDOW_STATE_MAXIMIZED  = 2,    // Fen�tre maximis�e
                WINDOW_STATE_FULLSCREEN = 3     // Mode plein �cran
            };

            /**
             * Window state changed
             */
            virtual void StateChanged(E_WINDOW_STATE window_state) = 0;

            /**
             * Window has benn resized
             */
            virtual void SizeChanged(Area<uint32_t> size) = 0;

            /**
             * Virtual destructor
             */
            virtual ~IWindowListener(){}
    };
}
