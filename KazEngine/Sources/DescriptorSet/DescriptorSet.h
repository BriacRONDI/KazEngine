#pragma once

#include "../Vulkan/Vulkan.h"
#include "../DataBank/DataBank.h"

namespace Engine
{
    class DescriptorSet
    {
        public :

            struct BINDING_INFOS {
                VkDescriptorType type;
                VkShaderStageFlags stage;
                VkDeviceSize size;
            };
            
            DescriptorSet();
            inline DescriptorSet(DescriptorSet&& other) { *this = std::move(other); }
            DescriptorSet& operator=(DescriptorSet&& other);
            inline ~DescriptorSet() { this->Clear(); }
            void Clear();
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            bool Create(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool Create(Tools::IMAGE_MAP const& image);
            bool Prepare(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, uint32_t instance_count = 1);
            bool Create(std::vector<BINDING_INFOS> infos, bool instanced = true, std::vector<std::shared_ptr<Chunk>> external_chunk = {});
            uint32_t Allocate(std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool PrepareBindlessTexture(uint32_t texture_count = 32);
            int32_t AllocateTexture(Tools::IMAGE_MAP const& image);
            inline VkDescriptorSet Get(uint32_t instance_id = 0) const { return this->sets[instance_id]; }
            inline VkDescriptorSetLayout GetLayout() const { return this->layout; }
            inline std::shared_ptr<Chunk> GetChunk(uint8_t binding = 0) const { return this->bindings[binding].chunk; }

            void WriteData(const void* data, VkDeviceSize data_size, size_t relative_offset, uint8_t binding, uint8_t instance_id);
            // { DataBank::GetInstancedBuffer().WriteData(data, data_size, this->bindings[binding].chunk->offset + relative_offset, instance_id); }

            inline void WriteData(const void* data, VkDeviceSize data_size, size_t relative_offset, uint8_t binding = 0)
            { for(uint8_t i=0; i<this->sets.size(); i++) { this->WriteData(data, data_size, relative_offset, binding, i); } }

            void ReadData(uint8_t binding = 0, uint8_t instance_id = 0);
            // { DataBank::GetInstancedBuffer().ReadData(this->bindings[binding].chunk, instance_id); }

            void AcquireFromStaging(void* data, VkDeviceSize data_size, size_t relative_offset, uint8_t binding, uint8_t instance_id);
            // { DataBank::GetInstancedBuffer().ReadStagingBuffer(data, data_size, this->bindings[binding].chunk->offset + relative_offset, instance_id); }

            // inline void* DirectStagingAccess(size_t relative_offset, uint8_t binding, uint8_t instance_id)
            void* DirectStagingAccess(uint8_t instance_id = 0, uint8_t binding = 0, size_t relative_offset = 0);

            inline std::shared_ptr<Chunk> ReserveRange(size_t size, uint8_t binding = 0) { return this->bindings[binding].chunk->ReserveRange(size); }
            std::shared_ptr<Chunk> ReserveRange(size_t size, size_t alignment, uint8_t binding = 0);
            bool ResizeChunk(std::shared_ptr<Chunk> chunk, size_t size, uint8_t binding = 0, VkDeviceSize alignment = 0);
            bool ResizeChunk(size_t size, uint8_t binding = 0, VkDeviceSize alignment = 0);
            inline void Update() { for(uint8_t i=0; i<this->sets.size(); i++) this->Update(i); }
            bool Update(uint8_t instance_id);
            bool NeedUpdate(uint8_t instance_id) const;
            inline void FreeChunk(std::shared_ptr<Chunk> chunk, uint8_t binding) { this->bindings[binding].chunk->FreeChild(chunk); }
            inline void AwaitUpdate(uint8_t binding) { for(uint8_t i=0; i<this->bindings[binding].awaiting_update.size(); i++) this->bindings[binding].awaiting_update[i] = true; }

        private :

            struct DESCRIPTOR_SET_BINDING {
                VkDescriptorSetLayoutBinding layout;
                std::shared_ptr<Chunk> chunk;
                std::vector<bool> awaiting_update;
            };

            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSet> sets;
            VkSampler sampler;
            std::vector<Vulkan::IMAGE_BUFFER> image_buffers;
            std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
            std::vector<DESCRIPTOR_SET_BINDING> bindings;

            bool CreateSampler();
            void UpdateDescriptorSet(uint8_t binding, uint8_t instance_id = 0);
    };
}