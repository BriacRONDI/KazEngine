#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../DataBank/DataBank.h"
#include "../../Camera/Camera.h"
#include "../Entity/Entity.h"
#include "../../Drawable/Drawable.h"

namespace Engine
{
    class EntityRender
    {
        public :
            
            void Clear();
            inline ~EntityRender() { this->Clear(); }
            EntityRender(VkCommandPool command_pool);
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            
            bool AddEntity(Entity& entity);
            void Update(uint8_t frame_index);

        private :

            struct SKELETON_BUFFER_INFOS {
                uint32_t skeleton_dynamic_offset;
                uint32_t animations_data_dynamic_offset;
                std::map<std::string, std::pair<uint32_t, uint32_t>> dynamic_offsets; // Key = mesh name, Values = [first] : bone offsets, [second] : offsets ids
            };

            struct DRAWABLE_BIND {
                Drawable mesh;
                // uint32_t entity_ids_dynamic_offset;
                // uint32_t skeleton_ids_dynamic_offset;
                // uint32_t skeleton_offsets_dynamic_offset;
                uint32_t texture_id;
                std::vector<uint32_t> dynamic_offsets;
                bool has_skeleton;
                std::vector<Entity*> entities;
            };

            struct RENDER_GOURP {
                Vulkan::PIPELINE pipeline;
                uint16_t mask;
                std::vector<DRAWABLE_BIND> drawables;
            };

            Vulkan::DATA_CHUNK entity_ids_chunk;
            Vulkan::DATA_CHUNK entity_data_chunk;
            Vulkan::DATA_CHUNK skeleton_bones_chunk;
            Vulkan::DATA_CHUNK skeleton_offsets_chunk;
            Vulkan::DATA_CHUNK skeleton_offsets_ids_chunk;
            Vulkan::DATA_CHUNK skeleton_animations_chunk;

            DescriptorSet texture_descriptor;
            DescriptorSet entities_descriptor;
            DescriptorSet skeleton_descriptor;

            VkCommandPool command_pool;
            std::vector<VkCommandBuffer> command_buffers;
            std::vector<RENDER_GOURP> render_groups;
            std::vector<Entity*> entities;
            std::map<std::string, SKELETON_BUFFER_INFOS> skeletons;
            std::map<std::string, uint32_t> textures;
            

            bool LoadTexture(std::string name);
            bool LoadSkeleton(std::string name);
            bool SetupDescriptorSets();
            bool SetupPipelines();
    };
}