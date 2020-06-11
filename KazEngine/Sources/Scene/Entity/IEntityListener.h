#pragma once

namespace Engine
{
    class IEntityListener
    {
        public :

            virtual void StaticBufferUpdated() = 0;
    };
}