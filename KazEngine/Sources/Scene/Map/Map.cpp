#include "Map.h"

namespace Engine
{
    void Map::Clear()
    {
        this->need_update.clear();

        if(Vulkan::HasInstance()) {

            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Chunks
            // DataBank::GetManagedBuffer().FreeChunk(this->entity_ids_chunk);
            DataBank::GetManagedBuffer().FreeChunk(this->map_vbo_chunk);

            // Descriptor Sets
            // this->entities_descriptor.Clear();
            this->texture_descriptor.Clear();

            // Pipeline
            this->pipeline.Clear();

            // Secondary graphics command buffers
            for(auto command_buffer : this->command_buffers)
                if(command_buffer != nullptr) vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, 1, &command_buffer);
        }

        this->index_buffer_offet = 0;
    }

    Map::Map(VkCommandPool command_pool) : command_pool(command_pool)
    {
        this->Clear();

        // Entity::AddListener(this);

        uint32_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->need_update.resize(frame_count, true);

        // Allocate draw command buffers
        this->command_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(command_pool, this->command_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Map::Map() => AllocateCommandBuffer [draw] : Failed" << std::endl;
            #endif
        }

        this->map_vbo_chunk = DataBank::GetManagedBuffer().ReserveChunk(SIZE_KILOBYTE(5));

        this->SetupDescriptorSets();
        this->CreatePipeline();
    }

    bool Map::SetupDescriptorSets()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        uint8_t instance_count = DataBank::GetManagedBuffer().GetInstanceCount();

        /////////////
        // TEXTURE //
        /////////////

        auto data_buffer = Tools::GetBinaryFileContents("data2.kea");
        auto image = DataBank::GetImageFromPackage(data_buffer, "/grass_tile2");
        this->texture_descriptor.Create(image);

        ///////////////
        // Selection //
        ///////////////

        if(!this->selection_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t) * 101},
        }, instance_count)) return false;

        this->selection_chunk = this->selection_descriptor.ReserveRange(sizeof(uint32_t) * 101);

        return true;
    }

    bool Map::CreatePipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<VkVertexInputBindingDescription> vertex_binding_description;
        auto vertex_attribute_description = Vulkan::CreateVertexInputDescription({{Vulkan::POSITION, Vulkan::UV}}, vertex_binding_description);

        auto camera_layout = Camera::GetInstance().GetDescriptorSet().GetLayout();
        auto texture_layout = this->texture_descriptor.GetLayout();
        auto selection_layout = this->selection_descriptor.GetLayout();
        auto entity_layout = DynamicEntity::GetMatrixDescriptor().GetLayout();

        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {camera_layout, texture_layout, selection_layout, entity_layout},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        return true;
    }

    bool Map::UpdateVertexBuffer(uint8_t frame_index)
    {
        struct SHADER_INPUT {
            Maths::Vector3 position;
            Maths::Vector2 uv;
        };

        float far_clip = 2000.0f;
        std::vector<SHADER_INPUT> vertex_data(8);
        vertex_data[0].position = { -far_clip, 0.0f, -far_clip };
        vertex_data[0].uv = { 0.0f, far_clip };
        vertex_data[1].position = { -far_clip, 0.0f, far_clip };
        vertex_data[1].uv = { 0.0f, 0.0f };
        vertex_data[2].position = { far_clip, 0.0f, far_clip };
        vertex_data[2].uv = { far_clip, 0.0f };
        vertex_data[3].position = { far_clip, 0.0f, -far_clip };
        vertex_data[3].uv = { far_clip, far_clip };

        this->index_buffer_offet = static_cast<uint32_t>(vertex_data.size()) * sizeof(SHADER_INPUT);
        DataBank::GetManagedBuffer().WriteData(vertex_data.data(), this->index_buffer_offet, this->map_vbo_chunk->offset, frame_index);

        std::vector<uint32_t> index_buffer = {0, 2, 1, 0, 3, 2};
        DataBank::GetManagedBuffer().WriteData(index_buffer.data(), index_buffer.size() * sizeof(uint32_t),
                                               this->map_vbo_chunk->offset + this->index_buffer_offet, frame_index);

        return true;
    }

    void Map::UpdateSelection(std::vector<DynamicEntity*> entities)
    {
        this->selected_entities = std::move(entities);
        uint32_t count = static_cast<uint32_t>(this->selected_entities.size());
        
        if(this->selection_chunk->range < (count + 1) * sizeof(uint32_t)) {
            if(!this->selection_descriptor.ResizeChunk(this->selection_chunk, (count + 1) * sizeof(uint32_t), 0, Vulkan::SboAlignment())) {
                count = static_cast<uint32_t>(this->selection_chunk->range / sizeof(uint32_t)) - 1;
                #if defined(DISPLAY_LOGS)
                std::cout << "Map::UpdateSelection : Not engough memory" << std::endl;
                #endif
            }
        }

        this->selection_descriptor.WriteData(&count, sizeof(uint32_t), static_cast<uint32_t>(this->selection_chunk->offset));

        std::vector<uint32_t> selected_ids(count);
        for(uint32_t i=0; i<count; i++) selected_ids[i] = this->selected_entities[i]->GetInstanceId();

        this->selection_descriptor.WriteData(selected_ids.data(), selected_ids.size() * sizeof(uint32_t),
                                             static_cast<uint32_t>(this->selection_chunk->offset + sizeof(uint32_t)));
    }

    Maths::Vector3 Map::GetMouseRayPosition()
    {
        return {};
    }

    void Map::Update(uint8_t frame_index)
    {
        if(!this->need_update[frame_index]) return;
        this->UpdateVertexBuffer(frame_index);
    }

    void Map::UpdateDescriptorSet(uint8_t frame_index)
    {
        if(this->selection_descriptor.NeedUpdate(frame_index)) {
            this->Refresh(frame_index);
            this->selection_descriptor.Update(frame_index);
        }
    }

    void Map::Refresh(uint8_t frame_index)
    {
        this->need_update[frame_index] = true;
        vkResetCommandBuffer(this->command_buffers[frame_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    VkCommandBuffer Map::GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        if(!this->need_update[frame_index]) return command_buffer;
        this->need_update[frame_index] = false;

        VkCommandBufferInheritanceInfo inheritance_info = {};
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = framebuffer;
        inheritance_info.renderPass = Vulkan::GetRenderPass();

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        command_buffer_begin_info.pInheritanceInfo = &inheritance_info;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildRenderPass[Map] => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        auto& surface = Vulkan::GetDrawSurface();
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(surface.width);
        viewport.height = static_cast<float>(surface.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = surface.width;
        scissor.extent.height = surface.height;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        std::vector<VkDescriptorSet> bind_descriptor_sets = {
            Camera::GetInstance().GetDescriptorSet().Get(frame_index),
            this->texture_descriptor.Get(),
            this->selection_descriptor.Get(frame_index),
            DynamicEntity::GetMatrixDescriptor().Get(frame_index)
        };

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);

        // uint32_t dynamic_offset = static_cast<uint32_t>(this->selection_chunk->offset);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(),
                                0, nullptr);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &DataBank::GetManagedBuffer().GetBuffer(frame_index).handle, &this->map_vbo_chunk->offset);
        vkCmdBindIndexBuffer(command_buffer, DataBank::GetManagedBuffer().GetBuffer(frame_index).handle,
                             this->map_vbo_chunk->offset + this->index_buffer_offet, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildMainRenderPass[Map] => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        // Succès
        return command_buffer;
    }
}