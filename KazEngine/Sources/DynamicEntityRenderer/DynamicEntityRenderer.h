#pragma once

#include <Singleton.hpp>
#include "../Vulkan/Vulkan.h"
#include "../GlobalData/GlobalData.h"
#include "../LOD/LOD.h"
#include "../DynamicEntity/DynamicEntity.h"
#include "../InstancedDescriptorSet/IInstancedDescriptorListener.h"
#include "../MappedDescriptorSet/IMappedDescriptorListener.h"

namespace Engine
{
    class DynamicEntityRenderer : public Singleton<DynamicEntityRenderer>, public IInstancedDescriptorListener, public IMappedDescriptorListener
    {
            friend class Singleton<DynamicEntityRenderer>;

        public :

            VkCommandBuffer BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            bool AddToScene(DynamicEntity& entity);
            uint32_t GetLodCount() const { return this->lod_count; }
            void Refresh() { std::fill(this->refresh.begin(), this->refresh.end(), true); }
            std::vector<DynamicEntity*> const& GetEntities() const { return this->entities; }

            // IInstancedDescriptorListener
            void InstancedDescriptorSetUpdated(InstancedDescriptorSet* descriptor, uint8_t binding) { this->Refresh(); }

            // IMappedDescriptorListener
            void MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding) { this->Refresh(); }

        private :

            VkCommandPool command_pool;
            std::vector<bool> refresh;
            std::vector<VkCommandBuffer> command_buffers;
            vk::PIPELINE pipeline;
            uint32_t lod_count;

            std::vector<DynamicEntity*> entities;

            DynamicEntityRenderer();
            ~DynamicEntityRenderer();
    };
}