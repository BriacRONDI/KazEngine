#pragma once

#include <map>
#include "../Vulkan.h"

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

            /*struct DESCRIPTOR_UPDATE {
                VkImageView imageView;
                VkDescriptorBufferInfo pBufferInfo;
            };*/

            enum LAYOUT_ARRAY_TYPE {
                ENTITY_ONLY_LAYOUT_ARRAY        = 0,
                TEXTURE_LAYOUT_ARRAY            = 1,
                SKELETON_TEXTURE_LAYOUT_ARRAY   = 2,
                SKELETON_LAYOUT_ARRAY           = 3
            };

            DescriptorSetManager();
            inline ~DescriptorSetManager(){ this->Clear(); }
            // bool Initialize(std::vector<VkDescriptorSetLayoutBinding> const& bindings);
            void Clear();
            // bool UpdateDescriptorSet(std::vector<DESCRIPTOR_UPDATE> const& updates, std::string const& texture = {});
            // inline VkDescriptorSet const GetDescriptorSet() const { return this->base_descriptor_set; }
            // inline VkDescriptorSet const GetDescriptorSet(std::string const& texture) const { return this->texture_sets.at(texture); }
            // inline bool HasTexture(std::string const& texture) const { return this->texture_sets.count(texture); }
            // inline VkDescriptorSetLayout const& GetLayout() const {return this->layout;}

            bool CreateViewDescriptorSet(VkDescriptorBufferInfo& camera_buffer, VkDescriptorBufferInfo& entity_buffer,
                                         VkDescriptorBufferInfo& lights_buffer, bool enable_geometry_shader = false);
            bool CreateSkeletonDescriptorSet(VkDescriptorBufferInfo& skeleton_buffer, VkDescriptorBufferInfo& mesh_buffer, bool enable_geometry_shader = false);
            bool CreateTextureDescriptorSet(VkImageView const view = nullptr, std::string const& texture = {});
            // bool CreateLightDescriptorSet(VkDescriptorBufferInfo& light_buffer);
            std::vector<VkDescriptorSetLayout> const GetLayoutArray(LAYOUT_ARRAY_TYPE type);

            inline VkDescriptorSet const GetViewDescriptorSet() { return this->view_set; }
            inline VkDescriptorSet const GetSkeletonDescriptorSet() { return this->skeleton_set; }
            inline VkDescriptorSet const GetTextureDescriptorSet(std::string const& texture) { return this->texture_sets[texture]; }

        private :

            // Définition des connexions entre descriptor sets et shaders
            // std::vector<VkDescriptorSetLayoutBinding> bindings;

            // Ressources  communes des descriptor sets
            /*VkDescriptorSetLayout layout;
            VkSampler sampler;
            VkDescriptorPool pool;*/

            // Sampler
            VkSampler sampler;

            // Descriptor set pointant sur le uniform buffer de la camera et des entités
            VkDescriptorPool view_pool;
            VkDescriptorSetLayout view_layout;
            VkDescriptorSet view_set;

            // Descriptor set pointant sur le uniform buffer des squelettes et des offsets des mesh
            VkDescriptorPool skeleton_pool;
            VkDescriptorSetLayout skeleton_layout;
            VkDescriptorSet skeleton_set;

            // Descriptor set avec texture
            VkDescriptorPool texture_pool;
            VkDescriptorSetLayout texture_layout;
            std::map<std::string, VkDescriptorSet> texture_sets;

            // Descriptor set de lumières
            VkDescriptorPool light_pool;
            VkDescriptorSetLayout light_layout;
            VkDescriptorSet light_set;

            // Helpers
            bool CreateSampler();
            // bool CreateDescriptorSetLayout();
            // bool CreatePool();
            // bool AllocateDescriptorSet(std::string const& texture = {});
            
    };
}