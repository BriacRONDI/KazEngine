#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../Camera/Camera.h"
#include "../../DataBank/DataBank.h"
#include "../Entity/Entity.h"
#include <Maths.h>

namespace Engine
{
    class Map
    {
        public :

            /*struct MAP_UBO {
                Maths::Vector3 selection_position;
	            uint32_t display_selection;
	            Maths::Vector3 destination_position;
	            uint32_t display_destination;
            };*/

            struct UNITS_SELECTION {
                uint32_t count;
                std::vector<uint32_t> ids;
            };
            
            void Clear();
            Map(VkCommandPool command_pool, Chunk entity_data_chunk);
            inline ~Map() { this->Clear(); }
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            // inline MAP_UBO& GetUniformBuffer() { return this->properties; }
            void Update(uint8_t frame_index);
            inline void Refresh() { for(int i=0; i<this->need_update.size(); i++) this->need_update[i] = true; }
            void UpdateSelection(UNITS_SELECTION selection);

        private :

            std::vector<bool> need_update;
            uint32_t index_buffer_offet;
            // MAP_UBO properties;
            // Vulkan::DATA_CHUNK map_ubo_chunk;
            Chunk map_vbo_chunk;
            Chunk entity_ids_chunk;
            Chunk entity_data_chunk;

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            // std::vector<DescriptorSet> descriptors;
            DescriptorSet texture_descriptor;
            DescriptorSet entities_descriptor;
            Vulkan::PIPELINE pipeline;
            UNITS_SELECTION selection;

            bool UpdateVertexBuffer(uint8_t frame_index);
            bool SetupDescriptorSets();
            bool CreatePipeline();
    };
}