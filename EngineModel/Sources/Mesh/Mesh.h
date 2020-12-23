#pragma once

#include <map>
#include <vector>
#include <string>
#include <Maths.h>
#include "../Deformer/Deformer.h"

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

namespace Model
{
    class Mesh
    {
        public :

            std::string name;
            std::vector<Maths::Vector3> vertex_buffer;
            std::vector<uint32_t> index_buffer;
            std::vector<Maths::Vector3> normal_buffer;
            std::vector<Maths::Vector2> uv_buffer;
            std::vector<uint32_t> uv_index;
            std::string texture;
            std::vector<Deformer> deformers;
            std::string skeleton;
            
            std::unique_ptr<char> BuildVBO(size_t& output_size, size_t& index_buffer_offset);
            std::unique_ptr<char> Serialize(uint32_t& output_size);
            size_t Deserialize(const char* data);
            void MirrorY();
            ~Mesh();

        protected :

            struct SERIALIZED_HEADER {
                uint8_t name_length;
                uint8_t skeleton_length;
                // uint32_t material_buffer_size;
                uint8_t texture_length;
                uint32_t vertex_count;
                uint32_t index_count;
                uint32_t uv_count;
                uint32_t uv_index_count;
                uint32_t deformer_count;
                bool normals;
            };

            // Helpers
            std::unique_ptr<char> BuildSubMeshVBO(size_t& output_size, size_t& index_buffer_offset);
            inline size_t GetVertexBufferSize() { return this->vertex_buffer.size() * sizeof(Maths::Vector3); }
            inline size_t GetUvBufferSize() { return this->uv_buffer.size() * sizeof(Maths::Vector2); }
            inline size_t GetUvIndexBufferSize() { return this->uv_index.size() * sizeof(uint32_t); }
            inline size_t GetIndexBufferSize() { return this->index_buffer.size() * sizeof(uint32_t); }
    };
}