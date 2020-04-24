#pragma once

#include "../../Vulkan/Vulkan.h"

namespace Engine
{
    class Map
    {
        public :
            
            Map(VkCommandPool command_pool);
            ~Map();
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index);

        private :

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;

            bool CreatePipeline();
    };
}