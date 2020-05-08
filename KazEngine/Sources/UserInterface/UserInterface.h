#pragma once

#include "../Vulkan/Vulkan.h"
#include "../DescriptorSet/DescriptorSet.h"
#include "../DataBank/DataBank.h"

namespace Engine
{
    class UserInterface
    {
        public :

            UserInterface(VkCommandPool command_pool);

        private :

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            Vulkan::PIPELINE pipeline;
            DescriptorSet texture_descriptor;

            Vulkan::DATA_CHUNK ui_vbo_chunk;

            bool SetupPipeline();
    };
}