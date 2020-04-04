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

            static inline DescriptorSetManager& GetInstance() { if(DescriptorSetManager::instance == nullptr) DescriptorSetManager::instance = new DescriptorSetManager; return *DescriptorSetManager::instance; }
            void DestroyInstance();
            void Clear();

            bool CreateSkeletonDescriptorSet(VkDescriptorBufferInfo const& meta_skeleton_buffer, VkDescriptorBufferInfo const& skeleton_buffer, VkDescriptorBufferInfo const& bone_offsets_buffer);
            bool CreateCameraDescriptorSet(VkDescriptorBufferInfo& camera_buffer, bool enable_geometry_shader = false);
            bool CreateEntityDescriptorSet(VkDescriptorBufferInfo& entity_buffer, bool enable_geometry_shader = false);
            bool CreateTextureDescriptorSet(Tools::IMAGE_MAP const& image, std::string const& texture);
            bool CreateMapDescriptorSet(VkDescriptorBufferInfo& map_buffer);

            bool InitializeTextureLayout();
            std::vector<VkDescriptorSetLayout> const GetLayoutArray(uint16_t schema);

            inline VkDescriptorSet const GetCameraDescriptorSet() { return this->camera_set; }
            inline VkDescriptorSet const GetEntityDescriptorSet() { return this->entity_set; }
            inline VkDescriptorSet const GetSkeletonDescriptorSet() { return this->skeleton_set; }
            inline VkDescriptorSet const GetMapDescriptorSet() { return this->map_set; }
            inline VkDescriptorSet const GetTextureDescriptorSet(std::string const& texture) { return this->texture_sets[texture].descriptor; }

        private :

            struct TEXTURE_DESCRIPTOR_SET {
                Vulkan::IMAGE_BUFFER buffer;
                VkDescriptorSet descriptor;
            };

            // Instance du singleton
            static DescriptorSetManager* instance;

            // Sampler
            VkSampler sampler;

            // Descriptor set pointant sur le uniform buffer de la map
            VkDescriptorPool map_pool;
            VkDescriptorSetLayout map_layout;
            VkDescriptorSet map_set;

            // Descriptor set pointant sur le uniform buffer de la camera
            VkDescriptorPool camera_pool;
            VkDescriptorSetLayout camera_layout;
            VkDescriptorSet camera_set;

            // Descriptor set pointant sur le uniform buffer des entités
            VkDescriptorPool entity_pool;
            VkDescriptorSetLayout entity_layout;
            VkDescriptorSet entity_set;

            // Descriptor set pointant sur le uniform buffer des squelettes
            VkDescriptorPool skeleton_pool;
            VkDescriptorSetLayout skeleton_layout;
            VkDescriptorSet skeleton_set;

            // Descriptor set avec texture
            VkDescriptorPool texture_pool;
            VkDescriptorSetLayout texture_layout;
            std::map<std::string, TEXTURE_DESCRIPTOR_SET> texture_sets;

            // Singleton
            DescriptorSetManager();
            inline ~DescriptorSetManager(){ this->Clear(); }

            // Helpers
            bool CreateSampler();
    };
}