#pragma once

#include <cstdint>
#include "MappedDescriptorSet.h"

namespace Engine
{
    class IMappedDescriptorListener
    {
        public :
            
            virtual void MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding) = 0;
    };
}