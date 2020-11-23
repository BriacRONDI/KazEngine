#pragma once

#include <cstdint>
#include "InstancedDescriptorSet.h"

namespace Engine
{
    class IInstancedDescriptorListener
    {
        public :
            
            virtual void InstancedDescriptorSetUpdated(InstancedDescriptorSet* descriptor, uint8_t binding) = 0;
    };
}