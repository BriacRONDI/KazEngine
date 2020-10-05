#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../../ManagedBuffer/ManagedBuffer.h"
#include "../../DataBank/DataBank.h"
#include "../../Camera/Camera.h"
#include "../../Entity/DynamicEntity/DynamicEntity.h"
#include "../../Entity/StaticEntity/StaticEntity.h"
#include "../../Entity/IEntityListener.h"
#include "../../LoadedMesh/LoadedMesh.h"
#include "../../Platform/Common/Timer/Timer.h"
#include "../../UnitControl/UnitControl.h"

#define ENTITY_ID_BINDING               0
#define ENTITY_DATA_BINDING             1

#define SKELETON_BONES_BINDING          0
#define SKELETON_OFFSET_IDS_BINDING     1
#define SKELETON_OFFSETS_BINDING        2
#define SKELETON_ANIMATIONS_BINDING     3


namespace Engine
{
    class EntityRender : public IEntityListener
    {
        public :
            
            void Clear();
            inline ~EntityRender() { this->Clear(); }
            EntityRender(VkCommandPool command_pool);
            VkCommandBuffer GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            inline void Refresh() { for(int i=0; i<this->need_graphics_update.size(); i++) this->Refresh(i); }
            void Refresh(uint8_t frame_index);
            VkSemaphore SubmitComputeShader(uint8_t frame_index, std::vector<VkSemaphore> wait_semaphores);
            void UpdateDescriptorSet(uint8_t frame_index);

            /////////////////////
            // IEntityListener //
            /////////////////////

            virtual void AddLOD(Entity& entity, std::shared_ptr<LODGroup> lod);
            virtual void NewEntity(Entity* entity) {};

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

            struct DRAWABLE_BIND;
            struct RENDER_GOURP {
                Vulkan::PIPELINE graphics_pipeline;
                Vulkan::PIPELINE compute_pipeline;
                uint16_t mask;
                std::vector<std::pair<bool, std::shared_ptr<Chunk>>> instance_buffer_chunks;
                DescriptorSet indirect_descriptor;
                std::vector<DRAWABLE_BIND> drawables;
                uint32_t instance_count;
            };

            struct DRAWABLE_BIND {
                std::shared_ptr<LODGroup> lod;
                uint32_t texture_id;
                std::shared_ptr<Chunk> chunk;
                std::vector<uint32_t> dynamic_offsets;
                bool has_skeleton;
                std::vector<ENTITY_MESH_CHUNK> entities;
                bool AddInstance(RENDER_GOURP& group, Entity& entity);
            };

            DescriptorSet texture_descriptor;
            DescriptorSet skeleton_descriptor;

            std::vector<bool> need_graphics_update;
            std::vector<bool> need_compute_update;
            VkCommandPool graphics_command_pool;
            VkCommandPool compute_command_pool;
            std::vector<VkCommandBuffer> graphics_command_buffers;
            std::vector<VkCommandBuffer> compute_command_buffers;
            std::vector<VkSemaphore> compute_semaphores;
            std::vector<RENDER_GOURP> render_groups;
            std::map<std::string, SKELETON_BUFFER_INFOS> skeletons;
            std::map<std::string, uint32_t> textures;
            // Vulkan::PIPELINE move_pipeline;

            bool LoadTexture(std::string name);
            bool LoadSkeleton(std::string name);
            bool SetupDescriptorSets();
            bool SetupPipelines();
            bool BuildComputeCommandBuffer(uint8_t frame_index);
    };
}