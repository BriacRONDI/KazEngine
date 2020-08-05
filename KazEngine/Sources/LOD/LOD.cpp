#include "LOD.h"

namespace Engine
{
    DescriptorSet LODGroup::lod_descriptor;

    bool LODGroup::Initialize()
    {
        if(!LODGroup::lod_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(LOD) * MAX_LOD_COUNT}
        }, Vulkan::GetConcurrentFrameCount())) return false;

        return true;
    }

    LODGroup::~LODGroup()
    {
        if(this->hit_box != nullptr) {
            delete this->hit_box;
            this->hit_box = nullptr;
        }
    }

    void LODGroup::Clear()
    {
        LODGroup::lod_descriptor.Clear();
    }

    void LODGroup::AddLOD(std::shared_ptr<Model::Mesh> mesh, uint8_t level)
    {
        if(this->meshes.size() >= MAX_LOD_COUNT) return;
        if(this->meshes.size() <= level) this->meshes.resize(level + 1);
        this->meshes[level] = mesh;
    }

    bool LODGroup::Build(std::map<std::string, uint32_t>& textures)
    {
        // Reserve LOD chunk
        this->lod_chunk = this->lod_descriptor.ReserveRange(sizeof(LOD) * MAX_LOD_COUNT);
        if(this->lod_chunk == nullptr) {
            if(!this->lod_descriptor.ResizeChunk(this->lod_descriptor.GetChunk()->range + sizeof(LOD) * MAX_LOD_COUNT, 0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "LODGroup::Build() : Not enough memory" << std::endl;
                #endif
                return false;
            }else{
                this->lod_chunk = this->lod_descriptor.ReserveRange(sizeof(LOD) * MAX_LOD_COUNT);
            }
        }

        // Set LOD chunk
        uint32_t first_vertex = 0;
        for(uint8_t i=0; i<MAX_LOD_COUNT; i++) {

            if(i >= this->meshes.size()) {
                LOD lod = {};
                this->lod_descriptor.WriteData(&lod, sizeof(LOD), this->lod_chunk->offset + i * sizeof(LOD));

            }else{
                LOD lod;
                lod.first_vertex = first_vertex;
                lod.vertex_count = static_cast<uint32_t>(this->meshes[i]->index_buffer.size());
                lod._pad0 = 0;
                if(!lod.vertex_count) lod.vertex_count = static_cast<uint32_t>(this->meshes[i]->vertex_buffer.size());
                lod.distance = i * 5.0f;
                first_vertex += lod.vertex_count;

                this->lod_descriptor.WriteData(&lod, sizeof(LOD), this->lod_chunk->offset + i * sizeof(LOD));
            }
        }

        // Compute vertex buffer size
        VkDeviceSize total_vbo_size = 0;
        std::vector<std::pair<std::unique_ptr<char>, size_t>> vbos;
        for(uint8_t i=0; i<this->meshes.size(); i++) {
            
            VkDeviceSize vbo_size, index_buffer_offset;
            std::unique_ptr<char> mesh_vbo = this->meshes[i]->BuildVBO(vbo_size, index_buffer_offset);
            total_vbo_size += vbo_size;
            vbos.push_back({std::move(mesh_vbo), vbo_size});
        }

        // Allocate vertex buffer
        this->vertex_buffer_chunk = DataBank::GetManagedBuffer().ReserveChunk(total_vbo_size);
        if(this->vertex_buffer_chunk == nullptr) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LODGroup::Build() : Not enough memory" << std::endl;
            #endif
            return false;
        }

        // Write vertex buffer
        size_t offset = 0;
        for(uint8_t i=0; i<vbos.size(); i++) {

            DataBank::GetManagedBuffer().WriteData(vbos[i].first.get(), vbos[i].second, this->vertex_buffer_chunk->offset + offset);
            offset += vbos[i].second;
        }

        for(auto& mesh_material : this->GetMeshMaterials()) {

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

        return true;
    }

    void LODGroup::Render(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineLayout layout, uint32_t instance_count,
                          std::vector<std::shared_ptr<Chunk>> instance_buffer_chunks, size_t indirect_offset) const
    {
        vkCmdPushConstants(command_buffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PUSH_CONSTANT_MATERIAL), &this->materials[0]);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &this->vertex_buffer_chunk->offset);

        std::vector<VkBuffer> buffers = std::vector<VkBuffer>(instance_buffer_chunks.size(), buffer);
        std::vector<VkDeviceSize> instance_offsets(instance_buffer_chunks.size());
        for(uint8_t i=0; i<instance_buffer_chunks.size(); i++) instance_offsets[i] = instance_buffer_chunks[i]->offset;
        vkCmdBindVertexBuffers(command_buffer, 1, static_cast<uint32_t>(instance_offsets.size()), buffers.data(), instance_offsets.data());

        vkCmdDrawIndirect(command_buffer, buffer, indirect_offset, instance_count, sizeof(INDIRECT_COMMAND));
    }
}