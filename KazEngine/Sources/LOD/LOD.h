#pragma once

#include <Model.h>
#include "../DataBank/DataBank.h"
#include "../DescriptorSet/DescriptorSet.h"

#define MAX_LOD_COUNT   5

namespace Engine
{
    class LODGroup
    {
        public :

            struct LOD {
                uint32_t first_vertex;
			    uint32_t vertex_count;
			    float distance;
			    uint32_t valid;
            };

            struct INDIRECT_COMMAND {
                uint32_t vertexCount;
                uint32_t instanceCount;
                uint32_t firstVertex;
                uint32_t firstInstance;
                uint32_t lodIndex;
            };

            struct PUSH_CONSTANT_MATERIAL {
                Maths::Vector4 ambient;
                Maths::Vector4 diffuse;
                Maths::Vector4 specular;
                float transparency;
                int32_t texture_id;
            };

            struct HIT_BOX {
                Maths::Vector3 near_left_bottom_point;
                Maths::Vector3 far_right_top_point;
            };

            ~LODGroup();
            void AddLOD(std::shared_ptr<Model::Mesh> mesh, uint8_t level);
            bool Build(std::map<std::string, uint32_t>& textures);
            inline uint16_t GetRenderMask() const { return this->meshes[0]->render_mask; }
            inline std::vector<std::pair<std::string, std::vector<uint32_t>>> const& GetMeshMaterials() const { return this->meshes[0]->materials; }
            inline std::string const& GetMeshSkeleton() const { return this->meshes[0]->skeleton; }
            inline std::shared_ptr<Model::Mesh> GetMesh(uint8_t level = 0) const { return this->meshes[level]; }
            inline void SetHitBox(HIT_BOX hit_box) { if(this->hit_box == nullptr) this->hit_box = new HIT_BOX; *this->hit_box = hit_box; }
            inline HIT_BOX* GetHitBox() const { return this->hit_box; }
            inline uint32_t GetLodIndex() const { return static_cast<uint32_t>(this->lod_chunk->offset / (sizeof(LOD) * MAX_LOD_COUNT)); }
            void Render(VkCommandBuffer command_buffer, uint32_t instance_id, VkPipelineLayout layout, uint32_t instance_count,
                        std::vector<std::pair<bool, std::shared_ptr<Chunk>>> instance_buffer_chunks, size_t indirect_offset) const;

            static bool Initialize();
            static void Clear();
            static inline DescriptorSet& GetDescriptor() { return LODGroup::lod_descriptor; }
            static inline bool UpdateDescriptor(uint8_t instance_id) { return LODGroup::lod_descriptor.Update(instance_id); }

        private :

            std::vector<PUSH_CONSTANT_MATERIAL> materials;
            std::shared_ptr<Chunk> lod_chunk;
            std::shared_ptr<Chunk> vertex_buffer;
            std::vector<std::shared_ptr<Model::Mesh>> meshes;
            std::shared_ptr<Chunk> vertex_buffer_chunk;
            HIT_BOX* hit_box = nullptr;

            static DescriptorSet lod_descriptor;
    };
}