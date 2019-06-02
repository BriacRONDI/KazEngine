#pragma once

#include <cstdint>
#include "EWindowState.h"
#include "../../../Core/Types.hpp"

namespace Engine
{
    /**
     * Cette interface permet à une classe d'écouter les évènements en provenance d'une fenêtre
     */
    class IWindowListener
    {
        public:
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
