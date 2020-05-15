#pragma once

#include <Model.h>
#include "../DataBank/DataBank.h"

namespace Engine
{
    class Drawable
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
            void Render(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineLayout layout, uint32_t instance_count);
            inline bool IsSameMesh(std::shared_ptr<Model::Mesh> mesh) { return this->mesh == mesh; }

        private :

            // std::map<std::string, PUSH_CONSTANT_MATERIAL> materials;
            std::vector<PUSH_CONSTANT_MATERIAL> materials;
            std::shared_ptr<Model::Mesh> mesh;
            uint32_t vertex_count               = 0;
            size_t index_buffer_offset          = 0;
            Chunk buffer_chunk;
    };
}