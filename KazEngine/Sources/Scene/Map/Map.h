#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../Camera/Camera.h"
#include "../../DataBank/DataBank.h"
#include "../Entity/Entity.h"
#include <Maths.h>

namespace Engine
{
    class Map // : public IEntityListener
    {
        public :

            void Clear();
            Map(VkCommandPool command_pool);
            inline ~Map() { this->Clear(); }
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            void Update(uint8_t frame_index);
            inline void Refresh() { for(int i=0; i<this->need_update.size(); i++) this->Refresh(i); }
            void Refresh(uint8_t frame_index);
            void UpdateSelection(std::vector<Entity*> entities);
            void UpdateDescriptorSet(uint8_t frame_index);
            
            /////////////////////
            // IEntityListener //
            /////////////////////

            // inline virtual void StaticBufferUpdated() { this->entities_descriptor.AwaitUpdate(1); }

        private :

            std::vector<bool> need_update;
            uint32_t index_buffer_offet;
            std::shared_ptr<Chunk> map_vbo_chunk;
            std::shared_ptr<Chunk> selection_chunk;

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            DescriptorSet texture_descriptor;
            DescriptorSet selection_descriptor;
            Vulkan::PIPELINE pipeline;

            bool UpdateVertexBuffer(uint8_t frame_index);
            bool SetupDescriptorSets();
            bool CreatePipeline();
    };
}