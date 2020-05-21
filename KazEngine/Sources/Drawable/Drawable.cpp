#include "Drawable.h"

namespace Engine
{
    bool Drawable::Load(std::shared_ptr<Model::Mesh> mesh, std::map<std::string, uint32_t>& textures)
    {
        VkDeviceSize vbo_size;
        this->mesh = mesh;
        std::unique_ptr<char> mesh_vbo = this->mesh->BuildVBO(vbo_size, this->index_buffer_offset);
        this->buffer_chunk = DataBank::GetManagedBuffer().ReserveChunk(vbo_size);
        if(this->index_buffer_offset != UINT64_MAX) this->index_buffer_offset += this->buffer_chunk->offset;
        this->vertex_count = static_cast<uint32_t>(mesh->index_buffer.size());
        if(!this->vertex_count) this->vertex_count = static_cast<uint32_t>(mesh->vertex_buffer.size());

        for(auto& mesh_material : mesh->materials) {

            if(!DataBank::HasMaterial(mesh_material.first)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Drawable::Load() Material[" << mesh_material.first << "] : Not in data bank" << std::endl;
                #endif
                return false;
            }

            PUSH_CONSTANT_MATERIAL new_material;
            new_material.ambient = DataBank::GetMaterial(mesh_material.first).ambient;
            new_material.diffuse = DataBank::GetMaterial(mesh_material.first).diffuse;
            new_material.specular = DataBank::GetMaterial(mesh_material.first).specular;
            new_material.transparency = DataBank::GetMaterial(mesh_material.first).transparency;

            if(!DataBank::GetMaterial(mesh_material.first).texture.empty()) {
                if(!textures.count(DataBank::GetMaterial(mesh_material.first).texture)) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "Drawable::Load() Texture[" << DataBank::GetMaterial(mesh_material.first).texture << "] : Not loaded" << std::endl;
                    #endif
                    return false;
                }else{
                    new_material.texture_id = textures[DataBank::GetMaterial(mesh_material.first).texture];
                }
            }else{
                new_material.texture_id = -1;
            }

            this->materials.push_back(new_material);
        }

        if(this->buffer_chunk == nullptr) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Drawable::Load->Model(" << this->mesh->name << ") => Failed" << std::endl;
            #endif
            return false;
        }

        DataBank::GetManagedBuffer().WriteData(mesh_vbo.get(), vbo_size, this->buffer_chunk->offset);

        return true;
    }

    void Drawable::Render(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineLayout layout, uint32_t instance_count)
    {
        if(!this->materials.empty())
            vkCmdPushConstants(command_buffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PUSH_CONSTANT_MATERIAL), &this->materials[0]);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &this->buffer_chunk->offset);

        if(this->index_buffer_offset != UINT64_MAX) {
            vkCmdBindIndexBuffer(command_buffer, buffer, this->index_buffer_offset, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(command_buffer, this->vertex_count, instance_count, 0, 0, 0);
        }else{
            vkCmdDraw(command_buffer, this->vertex_count, instance_count, 0, 0);
        }
    }
}