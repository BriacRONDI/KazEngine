#pragma once

namespace Engine
{
    class ITimerListener
    {
        public :

            virtual void Signal() = 0;
    };
}