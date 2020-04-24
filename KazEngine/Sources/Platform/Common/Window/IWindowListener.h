#pragma once

#include <cstdint>
#include <Types.hpp>
#include "../../Common/Window/Window.h"


namespace Engine
{
    /**
     * Cette interface permet à une classe d'écouter les évènements en provenance d'une fenêtre
     */
    class IWindowListener
    {
        public:

            enum E_WINDOW_STATE {
                WINDOW_STATE_WINDOWED   = 0,    // Fenêtre avec bords
                WINDOW_STATE_MINIMIZED  = 1,    // Fenêtre minimisée
                WINDOW_STATE_MAXIMIZED  = 2,    // Fenêtre maximisée
                WINDOW_STATE_FULLSCREEN = 3     // Mode plein écran
            };

            /**
             * Changement d'état
             */
            virtual void StateChanged(E_WINDOW_STATE window_state) = 0;

            /**
             * Redimentionnement de la fenêtre
             */
            virtual void SizeChanged(Area<uint32_t> size) = 0;

            /**
             * Destructeur virtuel
             */
            virtual ~IWindowListener(){}
    };
}
