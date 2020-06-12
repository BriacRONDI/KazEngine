#include "LoadedMesh.h"

namespace Engine
{
    bool LoadedMesh::Load(std::shared_ptr<Model::Mesh> mesh, std::map<std::string, uint32_t>& textures)
    {
        VkDeviceSize vbo_size;
        this->mesh = mesh;
        std::unique_ptr<char> mesh_vbo = this->mesh->BuildVBO(vbo_size, this->index_buffer_offset);
        this->vertex_buffer_chunk = DataBank::GetManagedBuffer().ReserveChunk(vbo_size);
        if(this->index_buffer_offset != UINT64_MAX) this->index_buffer_offset += this->vertex_buffer_chunk->offset;
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

        if(this->vertex_buffer_chunk == nullptr) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Drawable::Load->Model(" << this->mesh->name << ") => Failed" << std::endl;
            #endif
            return false;
        }

        DataBank::GetManagedBuffer().WriteData(mesh_vbo.get(), vbo_size, this->vertex_buffer_chunk->offset);

        return true;
    }

    VkDrawIndirectCommand LoadedMesh::GetIndirectCommand(uint32_t instance_id)
    {
        VkDrawIndirectCommand indirect;
        indirect.firstInstance = instance_id;
        indirect.firstVertex = 0;
        indirect.instanceCount = 1;
        indirect.vertexCount = this->vertex_count;
        return indirect;
    }

    void LoadedMesh::Render(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineLayout layout, uint32_t instance_count,
                            std::vector<std::shared_ptr<Chunk>> instance_buffer_chunks, size_t indirect_offset)
    {
        vkCmdPushConstants(command_buffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PUSH_CONSTANT_MATERIAL), &this->materials[0]);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &this->vertex_buffer_chunk->offset);

        /*std::vector<std::vector<VkDrawIndirectCommand>> staging_buffers(3);
        for(uint8_t j=0; j<3; j++) {
            std::vector<char> buffer(instance_count * sizeof(VkDrawIndirectCommand));
            DataBank::GetManagedBuffer().ReadStagingBuffer(buffer, indirect_offset, j);
            staging_buffers[j].resize(buffer.size() / sizeof(VkDrawIndirectCommand));
            std::memcpy(staging_buffers[j].data(), buffer.data(), buffer.size());
        }*/

        std::vector<VkBuffer> buffers = std::vector<VkBuffer>(instance_buffer_chunks.size(), buffer);
        std::vector<VkDeviceSize> instance_offsets(instance_buffer_chunks.size());
        for(uint8_t i=0; i<instance_buffer_chunks.size(); i++) instance_offsets[i] = instance_buffer_chunks[i]->offset;
        vkCmdBindVertexBuffers(command_buffer, 1, static_cast<uint32_t>(instance_offsets.size()), buffers.data(), instance_offsets.data());

        vkCmdDrawIndirect(command_buffer, buffer, indirect_offset, instance_count, sizeof(VkDrawIndirectCommand));
    }
}