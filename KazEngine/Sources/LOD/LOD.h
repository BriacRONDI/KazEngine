#pragma once

#include <Model.h>
#include "../GlobalData/GlobalData.h"

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

            LODGroup();
            ~LODGroup();
            void AddLOD(std::shared_ptr<Model::Mesh> lod, uint8_t level);
            bool Build();
            std::string const GetSkeleton() const { for(auto lod : this->lods) if(!lod->skeleton.empty()) return lod->skeleton; return {}; }
            std::string const GetTexture() const { for(auto lod : this->lods) if(!lod->texture.empty()) return lod->texture; return {}; }
            std::shared_ptr<Model::Mesh> GetLOD(uint8_t level = 0) const { return this->lods[level]; }
            void SetHitBox(HIT_BOX hit_box) { if(this->hit_box == nullptr) this->hit_box = new HIT_BOX; *this->hit_box = hit_box; }
            HIT_BOX* GetHitBox() const { return this->hit_box; }
            uint32_t GetLodIndex() const { return static_cast<uint32_t>(this->lod_chunk->offset / (sizeof(LOD) * MAX_LOD_COUNT)); }
            /*void Render(VkCommandBuffer command_buffer, uint32_t instance_id, VkPipelineLayout layout, uint32_t instance_count,
                        std::vector<std::pair<bool, std::shared_ptr<Chunk>>> instance_buffer_chunks, size_t indirect_offset, VkBuffer buffer) const;*/
            void SetTextureID(int32_t id) { this->texture_id = id; }
            // size_t GetVertexBufferOffset() { return this->vertex_buffer_chunk->offset; }

            // static bool Initialize();
            // static void Clear();
            // static InstancedDescriptorSet& GetDescriptor() { return *LODGroup::lod_descriptor; }

        private :

            std::shared_ptr<Chunk> lod_chunk;
            // std::shared_ptr<Chunk> vertex_buffer;
            std::vector<std::shared_ptr<Model::Mesh>> lods;
            std::shared_ptr<Chunk> vertex_buffer_chunk;
            int32_t texture_id;
            HIT_BOX* hit_box;
    };
}