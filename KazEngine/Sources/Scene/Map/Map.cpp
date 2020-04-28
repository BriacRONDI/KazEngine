#include "Map.h"

namespace Engine
{
    void Map::Clear()
    {
        this->need_update.clear();

        if(Vulkan::HasInstance()) {

            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Pipeline
            this->pipeline.Clear();

            // Staging buffers
            for(auto& staging_buffer : this->staging_buffers) staging_buffer.Clear();
            this->staging_buffers.clear();

            // Vertex buffers
            this->vertex_buffers.clear();

            // Transfer command buffers
            for(auto& buffer : this->transfer_buffers) {
                if(buffer.fence != nullptr) vkDestroyFence(Vulkan::GetDevice(), buffer.fence, nullptr);
                if(buffer.handle != nullptr) vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, 1, &buffer.handle);
            }

            // Secondary graphics command buffers
            for(auto command_buffer : this->command_buffers)
                if(command_buffer != nullptr) vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, 1, &command_buffer);
        }

        this->index_buffer_offet = 0;
    }

    Map::Map(VkCommandPool command_pool, std::vector<ManagedBuffer>& uniform_buffers) : command_pool(command_pool), uniform_buffers(uniform_buffers)
    {
        this->Clear();

        this->need_update.resize(3, true);

        // Allocate draw command buffers
        uint32_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->command_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(command_pool, this->command_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Map::Map() => AllocateCommandBuffer [draw] : Failed" << std::endl;
            #endif
        }

        // Allocate transfer command buffers
        this->transfer_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().CreateCommandBuffer(command_pool, this->transfer_buffers)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Map::Map() => CreateCommandBuffer [transfer] : Failed" << std::endl;
            #endif
        }

        std::vector<uint32_t> queue_families = {Vulkan::GetGraphicsQueue().index};
        if(Vulkan::GetGraphicsQueue().index != Vulkan::GetTransferQueue().index) queue_families.push_back(Vulkan::GetTransferQueue().index);
        this->staging_buffers.resize(frame_count);
        this->vertex_buffers.resize(frame_count);

        // Allocate vertex buffers
        for(uint8_t i=0; i<frame_count; i++) {

            // Allocate staging buffers
            if(!Vulkan::GetInstance().CreateDataBuffer(this->staging_buffers[i], 1024 * 1024 * 5,
                                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                       queue_families)) return;

            constexpr auto offset = 0;
            VkResult result = vkMapMemory(Vulkan::GetDevice(), this->staging_buffers[i].memory, offset, 1024 * 1024 * 5, 0, (void**)&this->staging_buffers[i].pointer);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Map::Map => vkMapMemory(staging_buffer) : Failed" << std::endl;
                #endif
                return;
            }

            // Allocate core buffers (vertex, uniform, index)
            if(!this->vertex_buffers[i].Create(this->staging_buffers[i], 1024 * 1024 * 5, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                return;
        }

        this->CreatePipeline();
    }

    bool Map::CreatePipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            DescriptorSet::CreateSimpleBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), // Camera
            DescriptorSet::CreateSimpleBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)  // Properties
        };

        this->map_descriptors.resize(this->uniform_buffers.size());
        for(uint8_t i=0; i<this->map_descriptors.size(); i++) {
        // for(auto& uniform_buffer : this->uniform_buffers) {

            std::vector<VkDescriptorBufferInfo> buffers_infos(2);
            buffers_infos[0].buffer = this->uniform_buffers[i].GetBuffer().handle;
            buffers_infos[0].offset = 0;
            buffers_infos[0].range = Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(Camera::CAMERA_UBO));

            buffers_infos[1].buffer = this->uniform_buffers[i].GetBuffer().handle;
            buffers_infos[1].offset = buffers_infos[0].range;
            buffers_infos[1].range = Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(MAP_UBO));

            this->map_descriptors[i].Create(bindings, buffers_infos);
        }

        VkVertexInputBindingDescription vertex_binding_description;
        auto vertex_attribute_description = Vulkan::CreateVertexInputDescription({Vulkan::POSITION, Vulkan::UV}, vertex_binding_description);
        
        auto data_buffer = Tools::GetBinaryFileContents("data.kea");
        auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
        auto image_package = DataPacker::Packer::FindPackedItem(data_tree, "/grass_tile2");
        auto image = Tools::LoadImageData(image_package.Data(data_buffer.data()), image_package.size);
        this->texture_descriptor.Create(image);

        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {this->texture_descriptor.GetLayout(), this->map_descriptors[0].GetLayout()},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        return true;
    }

    bool Map::UpdateVertexBuffer(ManagedBuffer& vertex_buffer, Vulkan::COMMAND_BUFFER const& command_buffer)
    {
        struct SHADER_INPUT {
            Maths::Vector3 position;
            Maths::Vector2 uv;
        };

        float far_clip = 2000.0f;
        std::vector<SHADER_INPUT> vertex_data(4);
        vertex_data[0].position = { -far_clip, 0.0f, -far_clip };
        vertex_data[0].uv = { 0.0f, far_clip };
        vertex_data[1].position = { -far_clip, 0.0f, far_clip };
        vertex_data[1].uv = { 0.0f, 0.0f };
        vertex_data[2].position = { far_clip, 0.0f, far_clip };
        vertex_data[2].uv = { far_clip, 0.0f };
        vertex_data[3].position = { far_clip, 0.0f, -far_clip };
        vertex_data[3].uv = { far_clip, far_clip };

        this->index_buffer_offet = static_cast<uint32_t>(vertex_data.size()) * sizeof(SHADER_INPUT);
        vertex_buffer.WriteData(vertex_data.data(), this->index_buffer_offet, 0);

        std::vector<uint32_t> index_buffer = {0, 2, 1, 0, 3, 2};
        vertex_buffer.WriteData(index_buffer.data(), index_buffer.size() * sizeof(uint32_t), this->index_buffer_offet);

        return vertex_buffer.Flush(command_buffer);
    }

    VkCommandBuffer Map::GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        if(!this->need_update[frame_index]) return command_buffer;

        ManagedBuffer& vertex_buffer = this->vertex_buffers[frame_index];
        this->UpdateVertexBuffer(vertex_buffer, this->transfer_buffers[frame_index]);
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
            this->texture_descriptor.Get(), this->map_descriptors[frame_index].Get()
        };

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(), 0, nullptr);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.GetBuffer().handle, &offset);
        vkCmdBindIndexBuffer(command_buffer, vertex_buffer.GetBuffer().handle, this->index_buffer_offet, VK_INDEX_TYPE_UINT32);
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