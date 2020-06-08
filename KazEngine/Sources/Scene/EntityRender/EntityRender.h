#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../DataBank/DataBank.h"
#include "../../Camera/Camera.h"
#include "../Entity/SkeletonEntity.h"
#include "../../LoadedMesh/LoadedMesh.h"

#define ENTITY_ID_BINDING   0
#define ENTITY_DATA_BINDING 1

namespace Engine
{
    class EntityRender
    {
        public :
            
            void Clear();
            inline ~EntityRender() { this->Clear(); }
            EntityRender(VkCommandPool command_pool);
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            std::vector<Entity*> SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end);
            Entity* ToggleSelection(Point<uint32_t> mouse_position);
            // inline DescriptorSet& GetEntityDescriptor() { return this->entities_descriptor; }
            inline void Refresh() { for(int i=0; i<this->need_update.size(); i++) this->Refresh(i); }
            void Refresh(uint8_t frame_index);
            
            bool AddEntity(Entity& entity);
            void Update(uint8_t frame_index);

        private :

            struct SKELETON_BUFFER_INFOS {
                uint32_t skeleton_dynamic_offset;
                uint32_t animations_data_dynamic_offset;
                std::map<std::string, std::pair<uint32_t, uint32_t>> dynamic_offsets; // Key = mesh name, Values = [first] : bone offsets, [second] : offsets ids
            };

            struct ENTITY_MESH_CHUNK {
                Entity* entity;
                std::shared_ptr<Chunk> chunk;
                uint32_t instance_id;
            };

            struct DRAWABLE_BIND {
                LoadedMesh mesh;
                uint32_t texture_id;
                std::shared_ptr<Chunk> chunk;
                std::vector<uint32_t> dynamic_offsets;
                bool has_skeleton;
                std::vector<ENTITY_MESH_CHUNK> entities;
            };

            struct RENDER_GOURP {
                Vulkan::PIPELINE pipeline;
                uint16_t mask;
                std::vector<std::shared_ptr<Chunk>> instance_buffer_chunks;
                std::shared_ptr<Chunk> indirect_commands_chunk;
                // uint32_t next_instance_id;
                std::vector<DRAWABLE_BIND> drawables;
            };

            // std::shared_ptr<Chunk> entity_data_chunk;
            // std::shared_ptr<Chunk> indirect_commands_chunk;

            std::shared_ptr<Chunk> instance_buffer_chunk;

            std::shared_ptr<Chunk> skeleton_bones_chunk;
            std::shared_ptr<Chunk> skeleton_offsets_chunk;
            std::shared_ptr<Chunk> skeleton_offsets_ids_chunk;
            std::shared_ptr<Chunk> skeleton_animations_chunk;

            DescriptorSet texture_descriptor;
            // DescriptorSet entities_descriptor;
            DescriptorSet skeleton_descriptor;

            std::vector<bool> need_update;
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