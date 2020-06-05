#pragma once

#include <Model.h>
#include "../DataBank/DataBank.h"

namespace Engine
{
    class LoadedMesh
    {
        public :

            struct PUSH_CONSTANT_MATERIAL {
                Maths::Vector4 ambient;
                Maths::Vector4 diffuse;
                Maths::Vector4 specular;
                float transparency;
                int32_t texture_id;
            };
            
            bool Load(std::shared_ptr<Model::Mesh> mesh, std::map<std::string, uint32_t>& textures);
            void Render(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineLayout layout, uint32_t instance_count,
                        std::vector<std::shared_ptr<Chunk>> instance_buffer_chunks, size_t indirect_offset);
            inline bool IsSameMesh(std::shared_ptr<Model::Mesh> mesh) { return this->mesh == mesh; }
            VkDrawIndirectCommand GetIndirectCommand(uint32_t instance_id);

        private :

            std::vector<PUSH_CONSTANT_MATERIAL> materials;
            std::shared_ptr<Model::Mesh> mesh;
            uint32_t vertex_count               = 0;
            size_t index_buffer_offset          = 0;
            std::shared_ptr<Chunk> vertex_buffer_chunk;
            uint32_t indirect_count;
    };
}