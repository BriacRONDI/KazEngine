#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../Camera/Camera.h"
#include "../../DataBank/DataBank.h"
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
            Map(VkCommandPool command_pool);
            inline ~Map() { this->Clear(); }
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            inline MAP_UBO& GetUniformBuffer() { return this->properties; }
            void Update(uint8_t frame_index);
            inline void Refresh() { for(int i=0; i<this->need_update.size(); i++) this->need_update[i] = true; }

        private :

            std::vector<bool> need_update;
            uint32_t index_buffer_offet;
            MAP_UBO properties;
            Vulkan::DATA_CHUNK map_ubo_chunk;
            Vulkan::DATA_CHUNK map_vbo_chunk;

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            std::vector<DescriptorSet> descriptors;
            DescriptorSet texture_descriptor;
            Vulkan::PIPELINE pipeline;

            bool UpdateVertexBuffer(uint8_t frame_index);
            bool CreatePipeline();
    };
}