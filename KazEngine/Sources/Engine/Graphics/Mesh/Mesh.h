#pragma once

#include <memory>
#include <map>

#include "../../Common/Packer/Packer.h"
#include "../Vulkan/Vulkan.h"
#include "../../Graphics/Renderer/Renderer.h"
#include "../../Graphics/Skeleton/Skeleton.h"

namespace Engine {

    struct DEFORMER;

    class Mesh {

        public :

            struct MATERIAL {
                Vector4 ambient;
                Vector4 diffuse;
                Vector4 specular;
                float transparency;
                std::string texture;
                
                static inline const uint32_t Size() { return sizeof(Vector4) * 3 + sizeof(float); }
                std::unique_ptr<char> Serialize(uint32_t& output_size);
                size_t Deserialize(const char* data);
                MATERIAL() : ambient({}), diffuse({}), specular({}), transparency(0.0f) {}
            };

            struct SUB_MESH {
                uint32_t index_count;
                uint32_t first_index;
            };

            struct HIT_BOX {
                Vector3 near_left_bottom_point;
                Vector3 far_right_top_point;
            };

            std::string name;
            std::vector<Vector3> vertex_buffer;
            std::vector<uint32_t> index_buffer;
            std::vector<Vector3> normal_buffer;
            std::vector<Vector2> uv_buffer;
            std::vector<uint32_t> uv_index;
            std::vector<std::pair<std::string, std::vector<uint32_t>>> materials;
            std::vector<std::pair<std::string, SUB_MESH>> sub_meshes;
            std::vector<DEFORMER> deformers;
            std::string skeleton;
            HIT_BOX hit_box;

            VkDeviceSize vertex_buffer_offset   = 0;
            VkDeviceSize index_buffer_offset    = 0;
            uint32_t skeleton_buffer_offset     = UINT32_MAX;
            uint16_t render_mask                = 0;

            inline VkDeviceSize GetVertexBufferSize() { return this->vertex_buffer.size() * sizeof(Vector3); }
            inline VkDeviceSize GetUvBufferSize() { return this->uv_buffer.size() * sizeof(Vector2); }
            inline VkDeviceSize GetUvIndexBufferSize() { return this->uv_index.size() * sizeof(uint32_t); }
            inline VkDeviceSize GetIndexBufferSize() { return this->index_buffer.size() * sizeof(uint32_t); }
            inline void UpdateIndexBufferOffset(VkDeviceSize move) { if(this->index_buffer_offset > 0) this->index_buffer_offset += move; }

            std::unique_ptr<char> BuildVBO(VkDeviceSize& output_size);
            
            void UpdateRenderMask(std::map<std::string, Mesh::MATERIAL> const& materials = {});
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
            std::unique_ptr<char> BuildSubMeshVBO(VkDeviceSize& output_size);
    };
}