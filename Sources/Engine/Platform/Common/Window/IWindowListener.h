#pragma once

#include <cstdint>
#include "EWindowState.h"
#include "../../../Core/Types.hpp"

namespace Engine
{
    /**
     * Cette interface permet � une classe d'�couter les �v�nements en provenance d'une fen�tre
     */
    class IWindowListener
    {
        public:
            /**
             * Changement d'�tat
             */
            virtual void StateChanged(E_WINDOW_STATE window_state) = 0;

            /**
             * Redimentionnement de la fen�tre
             */
            virtual void SizeChanged(Area<uint32_t> size) = 0;

            /**
             * Destructeur virtuel
             */
            virtual ~IWindowListener(){}
    };
}
