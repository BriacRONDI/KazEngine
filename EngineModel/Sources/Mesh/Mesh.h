#pragma once

#include <map>
#include <vector>

#include "../../../Maths/Sources/Maths.h"
#include "../Deformer/Deformer.h"

namespace Model
{

    class Mesh
    {
        public :

            struct MATERIAL {
                Maths::Vector4 ambient;
                Maths::Vector4 diffuse;
                Maths::Vector4 specular;
                float transparency;
                std::string texture;
                
                static inline const uint32_t Size() { return sizeof(Maths::Vector4) * 3 + sizeof(float); }
                std::unique_ptr<char> Serialize(uint32_t& output_size);
                size_t Deserialize(const char* data);
                MATERIAL() : transparency(0.0f) {}
            };

            struct SUB_MESH {
                uint32_t index_count;
                uint32_t first_index;
            };

            struct HIT_BOX {
                Maths::Vector3 near_left_bottom_point;
                Maths::Vector3 far_right_top_point;
            };

            std::string name;
            std::vector<Maths::Vector3> vertex_buffer;
            std::vector<uint32_t> index_buffer;
            std::vector<Maths::Vector3> normal_buffer;
            std::vector<Maths::Vector2> uv_buffer;
            std::vector<uint32_t> uv_index;
            std::vector<std::pair<std::string, std::vector<uint32_t>>> materials;
            std::vector<std::pair<std::string, SUB_MESH>> sub_meshes;
            std::vector<Deformer> deformers;
            std::string skeleton;
            HIT_BOX hit_box;

            size_t vertex_buffer_offset   = 0;
            size_t index_buffer_offset    = 0;
            uint32_t skeleton_buffer_offset     = UINT32_MAX;
            uint16_t render_mask                = 0;

            inline size_t GetVertexBufferSize() { return this->vertex_buffer.size() * sizeof(Maths::Vector3); }
            inline size_t GetUvBufferSize() { return this->uv_buffer.size() * sizeof(Maths::Vector2); }
            inline size_t GetUvIndexBufferSize() { return this->uv_index.size() * sizeof(uint32_t); }
            inline size_t GetIndexBufferSize() { return this->index_buffer.size() * sizeof(uint32_t); }
            inline void UpdateIndexBufferOffset(size_t move) { if(this->index_buffer_offset > 0) this->index_buffer_offset += move; }

            std::unique_ptr<char> BuildVBO(size_t& output_size);
            
            // void UpdateRenderMask(std::map<std::string, Mesh::MATERIAL> const& materials = {});
            std::unique_ptr<char> Serialize(uint32_t& output_size);
            size_t Deserialize(const char* data);

            #if defined(DISPLAY_LOGS)
            ~Mesh(){ std::cout << "Mesh [" << this->name << "] : deleted" << std::endl; }
            #else
            ~Mesh(){}
            #endif

        private :

            struct SERIALIZED_HEADER {
                uint8_t name_length;
                uint8_t skeleton_length;
                uint32_t material_buffer_size;
                uint32_t vertex_count;
                uint32_t index_count;
                uint32_t uv_count;
                uint32_t uv_index_count;
                uint32_t deformer_count;
                bool normals;
            };

            // Helpers
            std::unique_ptr<char> BuildSubMeshVBO(size_t& output_size);
    };
}