#pragma once

#include "../Vulkan/Vulkan.h"
#include "../../Graphics/Vulkan/DescriptorSetManager/DescriptorSetManager.h"
#include "../../Graphics/Mesh/Mesh.h"

namespace Engine
{
    class Renderer
    {
        public :

            enum SCHEMA_PRIMITIVE : uint16_t {
                INVALID_SCHEMA      = 0b0000'0000'0000'0000,
                POSITION_VERTEX     = 0b0000'0000'0000'0001,
                NORMAL_VERTEX       = 0b0000'0000'0000'0010,
                UV_VERTEX           = 0b0000'0000'0000'0100,
                COLOR_VERTEX        = 0b0000'0000'0000'1000,
                NORMAL_DEBUGGING    = 0b0000'0000'0001'0000,
                MATERIAL            = 0b0000'0000'0010'0000,
                TEXTURE             = 0b0000'0000'0100'0000,
                SKELETON            = 0b0000'0000'1000'0000
            };

            // Libération des ressources
            void Release();
            inline ~Renderer() { this->Release(); }

            // Initialisation du renderer compatible avec le schéma
            bool Initialize(const uint16_t schema, std::array<std::string, 3> const& shaders, std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts);

            // Récupère la pipeline
            inline Vulkan::PIPELINE const& GetPipeline() const { return this->pipeline; }

            // inline std::vector<VkDescriptorSetLayout> const& GetDescriptorSetLayouts() { return this->ds_layouts; }

            // Récupère le descriptor set
            // inline VkDescriptorSet const GetDescriptorSet() const { return this->descriptor_set.GetDescriptorSet(); }
            // inline VkDescriptorSet const GetDescriptorSet(std::string const& texture) const { return this->descriptor_set.GetDescriptorSet(texture); }
            // inline bool HasTexture(std::string const& texture) const { return this->descriptor_set.HasTexture(texture); }

            // Alias de fonction vers le DescriptorSetManager
            /*inline bool UpdateDescriptorSet(std::vector<DescriptorSetManager::DESCRIPTOR_UPDATE> const& updates, std::string const& texture = {}) {
                return this->descriptor_set.UpdateDescriptorSet(updates, texture);
            }*/

            // Indique si ce renderer est compatible avec le masque fourni
            // Cela permet de déterminer si cette pipeline peut afficher un modèle
            inline bool MatchRenderMask(uint16_t mask) { return this->render_mask == mask; }

        private :

            // Descriptor Set
            // DescriptorSetManager descriptor_set;

            // Descriptor Set Layouts
            // std::vector<VkDescriptorSetLayout> ds_layouts;

            // Pipeline
            Vulkan::PIPELINE pipeline;

            // Masque contenant les SCHEMA_PRIMITIVE, indiquant les capacités de rendu de la pipeline
            uint16_t render_mask;
    };
}