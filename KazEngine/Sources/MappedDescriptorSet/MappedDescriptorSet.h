#pragma once

#include <EventEmitter.hpp>
#include "../Vulkan/Vulkan.h"
#include "../Chunk/Chunk.h"

namespace Engine
{
    class IMappedDescriptorListener;

    class MappedDescriptorSet : public Tools::EventEmitter<IMappedDescriptorListener>
    {
        public :

            struct BINDING_INFOS {
                VkDescriptorType type;
                VkShaderStageFlags stage;
                VkDeviceSize size;

                BINDING_INFOS() : type(VK_DESCRIPTOR_TYPE_MAX_ENUM), stage(VK_SHADER_STAGE_ALL), size(0) {}
                BINDING_INFOS(VkDescriptorType type, VkShaderStageFlags stage, VkDeviceSize size = 0) : type(type), stage(stage), size(size) {}
            };
            
            MappedDescriptorSet();
            MappedDescriptorSet(MappedDescriptorSet&& other) { *this = std::move(other); }
            MappedDescriptorSet& operator=(MappedDescriptorSet&& other);
            ~MappedDescriptorSet() { this->Clear(); }
            void Clear();
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            bool Create(std::vector<BINDING_INFOS> infos);
            VkDescriptorSet Get() const { return this->set; }
            VkDescriptorSetLayout GetLayout() const { return this->layout; }
            std::shared_ptr<Chunk> GetChunk(uint8_t binding = 0) const { return this->bindings[binding].chunk; }
            std::shared_ptr<Chunk> ReserveRange(size_t size, uint8_t binding = 0);
            void WriteData(const void* data, size_t size, size_t offset, uint8_t binding = 0);
            void* AccessData(size_t offset, uint8_t binding = 0);
            void Update(uint8_t binding = 0);

            void FreeChunk(std::shared_ptr<Chunk> chunk, uint8_t binding) { this->bindings[binding].chunk->FreeChild(chunk); }

        private :

            struct DESCRIPTOR_SET_BINDING {
                VkDescriptorSetLayoutBinding layout;
                std::shared_ptr<Chunk> chunk;
            };

            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            VkDescriptorSet set;
            std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
            std::vector<DESCRIPTOR_SET_BINDING> bindings;

            size_t GetBindingAlignment(uint8_t binding);
    };
}