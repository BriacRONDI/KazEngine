#pragma once

#include "../../Graphics/Vulkan/Vulkan.h"

namespace Engine
{
    class Mesh
    {
        public:
            Mesh(uint32_t model_id);

        private:
            Vulkan::VULKAN_BUFFER uniform_buffer;
            uint32_t model_id;
            uint32_t offest;
    };
}