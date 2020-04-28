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
            
            void Clear();
            Map(VkCommandPool command_pool, std::vector<ManagedBuffer>& uniform_buffers);
            inline ~Map() { this->Clear(); }
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);

        private :

            std::vector<bool> need_update;
            uint32_t index_buffer_offet;

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            std::vector<Vulkan::COMMAND_BUFFER> transfer_buffers;
            std::vector<DescriptorSet> map_descriptors;
            DescriptorSet texture_descriptor;
            std::vector<ManagedBuffer>& uniform_buffers;
            Vulkan::PIPELINE pipeline;

            std::vector<Vulkan::STAGING_BUFFER> staging_buffers;
            std::vector<ManagedBuffer> vertex_buffers;

            bool UpdateVertexBuffer(ManagedBuffer& vertex_buffer, Vulkan::COMMAND_BUFFER const& command_buffer);
            bool CreatePipeline();
    };
}