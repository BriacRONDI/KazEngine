#pragma once

#include <map>
#include "../Vulkan.h"
#include "../../Renderer/Renderer.h"

namespace Engine
{
    class DescriptorSetManager
    {
        public :

            #if defined(DISPLAY_LOGS)
            static constexpr uint32_t TEXTURE_ALLOCATION_INCREMENT = 12;
            #else
            static constexpr uint32_t TEXTURE_ALLOCATION_INCREMENT = 10;
            #endif

            enum LAYOUT_ARRAY_TYPE {
                ENTITY_ONLY_LAYOUT_ARRAY        = 0,
                TEXTURE_LAYOUT_ARRAY            = 1,
                SKELETON_TEXTURE_LAYOUT_ARRAY   = 2,
                SKELETON_LAYOUT_ARRAY           = 3
            };

            DescriptorSetManager();
            inline ~DescriptorSetManager(){ this->Clear(); }
            void Clear();
            bool CreateViewDescriptorSet(VkDescriptorBufferInfo& camera_buffer, VkDescriptorBufferInfo& entity_buffer, bool enable_geometry_shader = false);
            bool CreateSkeletonDescriptorSet(VkDescriptorBufferInfo const& skeleton_buffer, VkDescriptorBufferInfo const& bone_offsets_buffer);
            bool CreateTextureDescriptorSet(VkImageView const view = nullptr, std::string const& texture = {});
            // bool CreateLightDescriptorSet(VkDescriptorBufferInfo& light_buffer);
            std::vector<VkDescriptorSetLayout> const GetLayoutArray(uint16_t schema);
            void UpdateSkeletonDescriptorSet(VkDescriptorBufferInfo const& skeleton_buffer, VkDescriptorBufferInfo const& bone_offsets_buffer);

            inline VkDescriptorSet const GetViewDescriptorSet() { return this->view_set; }
            inline VkDescriptorSet const GetSkeletonDescriptorSet() { return this->skeleton_set; }
            inline VkDescriptorSet const GetTextureDescriptorSet(std::string const& texture) { return this->texture_sets[texture]; }

        private :

            // Sampler
            VkSampler sampler;

            // Descriptor set pointant sur le uniform buffer de la camera et des entités
            VkDescriptorPool view_pool;
            VkDescriptorSetLayout view_layout;
            VkDescriptorSet view_set;

            // Descriptor set pointant sur le uniform buffer des squelettes
            VkDescriptorPool skeleton_pool;
            VkDescriptorSetLayout skeleton_layout;
            VkDescriptorSet skeleton_set;

            // Descriptor set avec texture
            VkDescriptorPool texture_pool;
            VkDescriptorSetLayout texture_layout;
            std::map<std::string, VkDescriptorSet> texture_sets;

            // Descriptor set de lumières
            /*VkDescriptorPool light_pool;
            VkDescriptorSetLayout light_layout;
            VkDescriptorSet light_set;*/

            // Helpers
            bool CreateSampler();
    };
}