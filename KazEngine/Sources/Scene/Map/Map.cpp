#include "Map.h"

namespace Engine
{
    void Map::Clear()
    {
        this->need_update.clear();
        this->properties = {};

        if(Vulkan::HasInstance()) {

            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Chunks
            DataBank::GetManagedBuffer().FreeChunk(this->map_ubo_chunk);
            DataBank::GetManagedBuffer().FreeChunk(this->map_vbo_chunk);

            // Descriptor Sets
            for(auto& descriptor : this->descriptors) descriptor.Clear();
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

        this->CreatePipeline();
    }

    bool Map::CreatePipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        size_t chunk_size = Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(MAP_UBO));
        this->map_ubo_chunk = DataBank::GetManagedBuffer().ReserveChunk(chunk_size);

        auto binding = DescriptorSet::CreateSimpleBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
        uint8_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->descriptors.resize(frame_count);
        for(uint8_t i=0; i<this->descriptors.size(); i++) 
            this->descriptors[i].Create({binding}, {DataBank::GetManagedBuffer().GetBufferInfos(this->map_ubo_chunk, i)});

        VkVertexInputBindingDescription vertex_binding_description;
        auto vertex_attribute_description = Vulkan::CreateVertexInputDescription({Vulkan::POSITION, Vulkan::UV}, vertex_binding_description);
        
        auto data_buffer = Tools::GetBinaryFileContents("data.kea");
        auto image = DataBank::GetImageFromPackage(data_buffer, "/grass_tile2");
        this->texture_descriptor.Create(image);

        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {Camera::GetInstance().GetDescriptorSet(0).GetLayout(), this->texture_descriptor.GetLayout(), this->descriptors[0].GetLayout()},
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
        DataBank::GetManagedBuffer().WriteData(vertex_data.data(), this->index_buffer_offet, this->map_vbo_chunk.offset, frame_index);

        std::vector<uint32_t> index_buffer = {0, 2, 1, 0, 3, 2};
        DataBank::GetManagedBuffer().WriteData(index_buffer.data(), index_buffer.size() * sizeof(uint32_t), this->map_vbo_chunk.offset + this->index_buffer_offet, frame_index);

        return true;
    }

    void Map::Update(uint8_t frame_index)
    {
        if(!this->need_update[frame_index]) return;
        this->UpdateVertexBuffer(frame_index);
        DataBank::GetManagedBuffer().WriteData(&this->properties, sizeof(MAP_UBO), this->map_ubo_chunk.offset, frame_index);
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


        auto camera_set = Camera::GetInstance().GetDescriptorSet(frame_index).Get();
        std::vector<VkDescriptorSet> bind_descriptor_sets = {camera_set, this->texture_descriptor.Get(), this->descriptors[frame_index].Get()};

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);

        // vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0, 1, &camera_set, 0, nullptr);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(), 0, nullptr);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &DataBank::GetManagedBuffer().GetBuffer(frame_index).handle, &this->map_vbo_chunk.offset);
        vkCmdBindIndexBuffer(command_buffer, DataBank::GetManagedBuffer().GetBuffer(frame_index).handle, this->map_vbo_chunk.offset + this->index_buffer_offet, VK_INDEX_TYPE_UINT32);
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