#include "DynamicEntityRenderer.h"

namespace Engine
{
    DynamicEntityRenderer::DynamicEntityRenderer()
    {
        this->lod_count = 0;
        this->refresh.resize(Vulkan::GetSwapChainImageCount(), true);
        this->command_buffers.resize(Vulkan::GetSwapChainImageCount());

        if(!vk::CreateCommandPool(this->command_pool, Vulkan::GetGraphicsQueue().index)) return;

        for(auto& command_buffer : this->command_buffers)
            if(!vk::CreateCommandBuffer(this->command_pool, command_buffer, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) return;

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
            vk::LoadShaderModule("./Shaders/dynamic_model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            vk::LoadShaderModule("./Shaders/textured_model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        std::vector<VkVertexInputBindingDescription> vertex_binding_description;
        std::vector<VkVertexInputAttributeDescription> vertex_attribute_description = vk::CreateVertexInputDescription({
            {vk::POSITION, vk::UV, vk::BONE_WEIGHTS, vk::BONE_IDS},
            {vk::MATRIX},
            {vk::UINT_ID, vk::UINT_ID}
        }, vertex_binding_description);

        bool success = vk::CreateGraphicsPipeline(
            true,
            {
                GlobalData::GetInstance()->texture_descriptor.GetLayout(),
                GlobalData::GetInstance()->camera_descriptor.GetLayout(),
                GlobalData::GetInstance()->skeleton_descriptor.GetLayout()
            },
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline
        );

        for(auto& stage : shader_stages) vk::Destroy(stage);

        GlobalData::GetInstance()->indirect_descriptor.AddListener(this);
        GlobalData::GetInstance()->skeleton_descriptor.AddListener(this);
        GlobalData::GetInstance()->dynamic_entity_descriptor.AddListener(this);
    }

    DynamicEntityRenderer::~DynamicEntityRenderer()
    {
        GlobalData::GetInstance()->dynamic_entity_descriptor.RemoveListener(this);
        GlobalData::GetInstance()->skeleton_descriptor.RemoveListener(this);
        GlobalData::GetInstance()->indirect_descriptor.RemoveListener(this);

        vk::Destroy(this->command_pool);
        vk::Destroy(this->pipeline);
    }

    bool DynamicEntityRenderer::AddToScene(DynamicEntity& entity)
    {
        for(auto lod : entity.GetModels()) {
            auto chunk = GlobalData::GetInstance()->indirect_descriptor.ReserveRange(sizeof(LODGroup::INDIRECT_COMMAND));
            if(chunk == nullptr) return false;

            LODGroup::INDIRECT_COMMAND indirect;
            indirect.firstInstance = entity.InstanceId();
            indirect.lodIndex = lod->GetLodIndex();
            indirect.firstVertex = 0;
            indirect.instanceCount = 1;
            indirect.vertexCount = 4500;
            GlobalData::GetInstance()->indirect_descriptor.WriteData(&indirect, sizeof(LODGroup::INDIRECT_COMMAND), static_cast<uint32_t>(chunk->offset));
            this->lod_count++;
        }

        this->entities.push_back(&entity);
        for(uint32_t i=0; i<this->refresh.size(); i++) this->refresh[i] = true;

        return true;
    }

    VkCommandBuffer DynamicEntityRenderer::BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        if(!this->refresh[frame_index]) return command_buffer;
        this->refresh[frame_index] = false;

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
            std::cout << "BuildRenderPass[Dynamics] => vkBeginCommandBuffer : Failed" << std::endl;
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

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);

        std::vector<VkDescriptorSet> bind_descriptor_sets = {
            GlobalData::GetInstance()->texture_descriptor.Get(),
            GlobalData::GetInstance()->camera_descriptor.Get(frame_index),
            GlobalData::GetInstance()->skeleton_descriptor.Get(frame_index)
        };

        vkCmdBindDescriptorSets(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
            static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(), 0, nullptr
        );

        std::vector<size_t> offsets = {
            GlobalData::GetInstance()->vertex_buffer->offset,
            GlobalData::GetInstance()->dynamic_entity_descriptor.GetChunk(ENTITY_MATRIX_BINDING)->offset,
            GlobalData::GetInstance()->dynamic_entity_descriptor.GetChunk(ENTITY_FRAME_BINDING)->offset
        };

        std::vector<VkBuffer> buffers = {
            GlobalData::GetInstance()->instanced_buffer.GetBuffer(frame_index).handle,
            GlobalData::GetInstance()->mapped_buffer.GetBuffer().handle,
            GlobalData::GetInstance()->mapped_buffer.GetBuffer().handle
        };

        vkCmdBindVertexBuffers(command_buffer, 0, static_cast<uint32_t>(offsets.size()), buffers.data(), offsets.data());

        vkCmdDrawIndirect(
            command_buffer,
            GlobalData::GetInstance()->instanced_buffer.GetBuffer(frame_index).handle,
            GlobalData::GetInstance()->indirect_descriptor.GetChunk()->offset,
            this->lod_count,
            sizeof(LODGroup::INDIRECT_COMMAND)
        );

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildMainRenderPass[Map] => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        return command_buffer;
    }
}