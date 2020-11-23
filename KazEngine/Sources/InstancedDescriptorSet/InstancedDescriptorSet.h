#pragma once

#include "../Vulkan/Vulkan.h"
#include "../Chunk/Chunk.h"

namespace Engine
{
    class IInstancedDescriptorListener;

    class InstancedDescriptorSet : public Tools::EventEmitter<IInstancedDescriptorListener>
    {
        public :

            struct BINDING_INFOS {
                VkDescriptorType type;
                VkShaderStageFlags stage;
                VkDeviceSize size;
            };
            
            InstancedDescriptorSet();
            InstancedDescriptorSet(InstancedDescriptorSet&& other) { *this = std::move(other); }
            InstancedDescriptorSet& operator=(InstancedDescriptorSet&& other);
            ~InstancedDescriptorSet() { this->Clear(); }
            void Clear();
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            bool Create(std::vector<BINDING_INFOS> infos);
            VkDescriptorSet Get(uint32_t instance_id = 0) const { return this->sets[instance_id]; }
            const VkDescriptorSetLayout GetLayout() const { return this->layout; }
            const std::shared_ptr<Chunk> GetChunk(uint8_t binding = 0) const { return this->bindings[binding].chunk; }
            std::shared_ptr<Chunk> ReserveRange(size_t size, uint8_t binding = 0);
            void WriteData(const void* data, VkDeviceSize size, size_t offset, uint8_t binding, uint8_t instance_id);
            void WriteData(const void* data, VkDeviceSize size, size_t offset, uint8_t binding = 0) { for(uint8_t i=0; i<this->sets.size(); i++) { this->WriteData(data, size, offset, binding, i); } }
            bool Update(uint8_t instance_id);

        private :

            struct DESCRIPTOR_SET_BINDING {
                VkDescriptorSetLayoutBinding layout;
                std::shared_ptr<Chunk> chunk;
                std::vector<bool> need_update;
            };

            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSet> sets;
            std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
            std::vector<DESCRIPTOR_SET_BINDING> bindings;
    };
}