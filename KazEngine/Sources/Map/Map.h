#pragma once

#include <Types.hpp>
#include "../Vulkan/Vulkan.h"
#include "../GlobalData/GlobalData.h"
#include "../InstancedDescriptorSet/IInstancedDescriptorListener.h"
#include "../MappedDescriptorSet/IMappedDescriptorListener.h"

namespace Engine
{
    class Map : public Singleton<Map>, public IInstancedDescriptorListener, public IMappedDescriptorListener
    {
        friend class Singleton<Map>;

        public :

            void Clear();
            bool Initialize();
            VkCommandBuffer BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            void Refresh() { std::fill(this->refresh.begin(), this->refresh.end(), true); }
            Area<uint32_t> const& GetSize() const { return this->size; }

            // IInstancedDescriptorListener
            void InstancedDescriptorSetUpdated(InstancedDescriptorSet* descriptor, uint8_t binding) { this->Refresh(); }

            // IMappedDescriptorListener
            void MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding) { this->Refresh(); }

        private :

            std::vector<bool> refresh;
            uint32_t index_buffer_offet;
            std::shared_ptr<Chunk> map_vbo_chunk;
            std::shared_ptr<Chunk> selection_chunk;
            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            vk::PIPELINE pipeline;
            Area<uint32_t> size;

            Map() : command_pool(nullptr), index_buffer_offet(0) {}
            ~Map() { this->Clear(); }

            bool UpdateVertexBuffer(uint8_t frame_index);
            bool SetupPipeline();
    };
}