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
            inline ~DescriptorSet() { this->Clear(); }
            void Clear();
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            // static std::vector<DESCRIPTOR_SET_BINDING> CreateBindings(std::vector<BINDING_INFOS> infos);
            bool Create(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool Create(Tools::IMAGE_MAP const& image);
            bool Prepare(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, uint32_t max_sets = 1);
            bool Create(std::vector<BINDING_INFOS> infos, uint32_t max_sets = 1);
            uint32_t Allocate(std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool PrepareBindlessTexture(uint32_t texture_count = 32);
            int32_t AllocateTexture(Tools::IMAGE_MAP const& image);
            inline VkDescriptorSet Get(uint32_t id = 0) { return this->sets[0]; }

            inline VkDescriptorSetLayout GetLayout() { return this->layout; }

            inline void WriteData(const void* data, VkDeviceSize data_size, uint8_t binding = 0, uint8_t instance_id = 0, uint32_t relative_offset = 0)
            { DataBank::GetInstance().GetManagedBuffer().WriteData(data, data_size, this->bindings[binding].chunks[instance_id].offset + relative_offset, instance_id); }

        private :

            struct DESCRIPTOR_SET_BINDING {
                VkDescriptorSetLayoutBinding layout;
                std::vector<Chunk> chunks;
            };

            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSet> sets;
            VkSampler sampler;
            std::vector<Vulkan::IMAGE_BUFFER> image_buffers;
            std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
            std::vector<DESCRIPTOR_SET_BINDING> bindings;

            bool CreateSampler();
    };
}