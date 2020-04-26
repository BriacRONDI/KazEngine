#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../Camera/Camera.h"
#include <DataPacker.h>
#include <Maths.h>

namespace Engine
{
    class Map
    {
        public :

            struct MAP_UBO {
                Maths::Vector3 selection_position;
	            uint32_t display_selection;
	            Maths::Vector3 destination_position;
	            uint32_t display_destination;
            };
            
            Map(VkCommandPool command_pool, std::vector<ManagedBuffer>& uniform_buffers);
            ~Map();
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer frame_buffer);

        private :

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            std::vector<DescriptorSet> map_descriptors;
            DescriptorSet texture_descriptor;
            std::vector<ManagedBuffer>& uniform_buffers;
            Vulkan::PIPELINE pipeline;

            bool CreatePipeline();
    };
}