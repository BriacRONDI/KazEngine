#include "LOD.h"

namespace Engine
{
    /*InstancedDescriptorSet* LODGroup::lod_descriptor = nullptr;

    bool LODGroup::Initialize()
    {
        if(LODGroup::lod_descriptor == nullptr) LODGroup::lod_descriptor = new InstancedDescriptorSet;

        if(!LODGroup::lod_descriptor->Create(instanced_buffer, {
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(LOD) * MAX_LOD_COUNT}
        })) return false;

        return true;
    }*/

    LODGroup::LODGroup()
    {
        this->texture_id = -1;
        this->hit_box = nullptr;
    }

    LODGroup::~LODGroup()
    {
        if(this->hit_box != nullptr) {
            delete this->hit_box;
            this->hit_box = nullptr;
        }
    }

    /*void LODGroup::Clear()
    {
        if(LODGroup::lod_descriptor != nullptr) {
            delete LODGroup::lod_descriptor;
            LODGroup::lod_descriptor = nullptr;
        }
    }*/

    void LODGroup::AddLOD(std::shared_ptr<Model::Mesh> mesh, uint8_t level)
    {
        if(this->lods.size() >= MAX_LOD_COUNT) return;
        if(this->lods.size() <= level) this->lods.resize(level + 1);
        this->lods[level] = mesh;
    }

    bool LODGroup::Build()
    {
        // Reserve LOD chunk
        this->lod_chunk = GlobalData::GetInstance()->lod_descriptor.ReserveRange(sizeof(LOD) * MAX_LOD_COUNT);
        if(this->lod_chunk == nullptr) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LODGroup::Build() : Not enough memory" << std::endl;
            #endif
            return false;
        }

        // Set LOD chunk
        uint32_t first_vertex = 0;
        float lod_distances[] = {0.0f, 15.0f, 40.0f, 100.0f};
        for(uint8_t i=0; i<MAX_LOD_COUNT; i++) {

            if(i >= this->lods.size()) {
                LOD lod = {};
                GlobalData::GetInstance()->lod_descriptor.WriteData(&lod, sizeof(LOD), this->lod_chunk->offset + i * sizeof(LOD));

            }else{
                LOD lod;
                lod.first_vertex = first_vertex;
                lod.vertex_count = static_cast<uint32_t>(this->lods[i]->index_buffer.size());
                lod.valid = 1;
                if(!lod.vertex_count) lod.vertex_count = static_cast<uint32_t>(this->lods[i]->vertex_buffer.size());
                lod.distance = lod_distances[i];
                first_vertex += lod.vertex_count;

                GlobalData::GetInstance()->lod_descriptor.WriteData(&lod, sizeof(LOD), this->lod_chunk->offset + i * sizeof(LOD));
            }
        }

        // Compute vertex buffer size
        VkDeviceSize total_vbo_size = 0;
        std::vector<std::pair<std::unique_ptr<char>, size_t>> vbos;
        for(uint8_t i=0; i<this->lods.size(); i++) {
            
            VkDeviceSize vbo_size, index_buffer_offset;
            std::unique_ptr<char> mesh_vbo = this->lods[i]->BuildVBO(vbo_size, index_buffer_offset);
            total_vbo_size += vbo_size;
            vbos.push_back({std::move(mesh_vbo), vbo_size});
        }

        // Allocate vertex buffer
        bool relocated;
        if(!GlobalData::GetInstance()->instanced_buffer.GetChunk()->ResizeChild(
                    GlobalData::GetInstance()->vertex_buffer,
                    GlobalData::GetInstance()->vertex_buffer->range + total_vbo_size,
                    relocated)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LODGroup::Build() : Not enough memory" << std::endl;
            #endif
            return false;
        }
        this->vertex_buffer_chunk = GlobalData::GetInstance()->vertex_buffer->ReserveRange(total_vbo_size);

        // Write vertex buffer
        size_t lod_offset = 0;
        for(uint8_t i=0; i<vbos.size(); i++) {

            GlobalData::GetInstance()->instanced_buffer.WriteData(
                vbos[i].first.get(), vbos[i].second,
                GlobalData::GetInstance()->vertex_buffer->offset + this->vertex_buffer_chunk->offset + lod_offset
            );
            lod_offset += vbos[i].second;
        }

        return true;
    }

    //void LODGroup::Render(VkCommandBuffer command_buffer, uint32_t instance_id, VkPipelineLayout layout, uint32_t instance_count,
    //                      std::vector<std::pair<bool, std::shared_ptr<Chunk>>> instance_buffer_chunks, size_t indirect_offset, VkBuffer buffer) const
    //{
    //    // VkBuffer buffer = DataBank::GetInstance()->instanced_buffer.GetBuffer(instance_id).handle;
    //    vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &this->vertex_buffer_chunk->offset);

    //    std::vector<VkBuffer> buffers = std::vector<VkBuffer>(instance_buffer_chunks.size(), buffer);
    //    std::vector<VkDeviceSize> instance_offsets(instance_buffer_chunks.size());
    //    for(uint8_t i=0; i<instance_buffer_chunks.size(); i++) {
    //        instance_offsets[i] = instance_buffer_chunks[i].second->offset;
    //        // if(!instance_buffer_chunks[i].first) buffers[i] = DataBank::GetInstance()->mapped_buffer.GetBuffer().handle;
    //    }
    //    vkCmdBindVertexBuffers(command_buffer, 1, static_cast<uint32_t>(instance_offsets.size()), buffers.data(), instance_offsets.data());

    //    vkCmdDrawIndirect(command_buffer, buffer, indirect_offset, instance_count, sizeof(INDIRECT_COMMAND));
    //}
}