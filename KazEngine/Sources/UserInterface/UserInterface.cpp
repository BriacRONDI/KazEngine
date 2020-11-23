#include "UserInterface.h"

namespace Engine
{
    UserInterface::UserInterface()
    {
        this->render_pass = nullptr;
    }

    void UserInterface::Clear()
    {
        Mouse::GetInstance().RemoveListener(this);
        GlobalData::GetInstance()->mouse_square_descriptor.RemoveListener(this);

        vk::Destroy(this->pipeline);
        vk::Destroy(this->render_pass);
        vk::Destroy(this->command_pool);

        this->refresh.clear();
        this->command_buffers.clear();
    }

    bool UserInterface::Initialize()
    {
        this->refresh.resize(Vulkan::GetSwapChainImageCount(), true);
        this->command_buffers.resize(Vulkan::GetSwapChainImageCount());

        if(!vk::CreateCommandPool(this->command_pool, Vulkan::GetGraphicsQueue().index)) {
            this->Clear();
            return false;
        }

        for(auto& command_buffer : this->command_buffers) {
            if(!vk::CreateCommandBuffer(this->command_pool, command_buffer)) {
                this->Clear();
                return false;
            }
        }

        if(!this->SetupRenderPass()) {
            this->Clear();
            return false;
        }

        if(!this->SetupPipeline()) {
            this->Clear();
            return false;
        }

        if(!this->SetupVertexBuffer()) {
            this->Clear();
            return false;
        }

        Mouse::GetInstance().AddListener(this);
        GlobalData::GetInstance()->mouse_square_descriptor.AddListener(this);

        return true;
    }

    bool UserInterface::SetupRenderPass()
    {
		VkAttachmentDescription attachment[2];

        attachment[0].format = Vulkan::GetSwapChain().format;
        attachment[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment[0].flags = 0;

        attachment[1].format = Vulkan::GetDepthFormat();
        attachment[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment[1].flags = 0;

        VkAttachmentReference color_reference = {};
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 1;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = &depth_reference;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        VkSubpassDependency dependency[2];

        dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	    dependency[0].dstSubpass = 0;
	    dependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	    dependency[1].srcSubpass = 0;
	    dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	    dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachment;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 2;
        rp_info.pDependencies = dependency;

        VkResult result = vkCreateRenderPass(Vulkan::GetDevice(), &rp_info, nullptr, &this->render_pass);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::SetupRenderPass() => vkCreateRenderPass : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    bool UserInterface::SetupPipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = vk::LoadShaderModule("./Shaders/interface.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = vk::LoadShaderModule("./Shaders/interface.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        size_t chunk_size = SIZE_KILOBYTE(5);
        this->ui_vbo_chunk = GlobalData::GetInstance()->instanced_buffer.GetChunk()->ReserveRange(chunk_size);

        std::vector<VkVertexInputBindingDescription> vertex_binding_description;
        auto vertex_attribute_description = vk::CreateVertexInputDescription({{vk::POSITION_2D, vk::UV}}, vertex_binding_description);

        bool success = vk::CreateGraphicsPipeline(
            true, {GlobalData::GetInstance()->mouse_square_descriptor.GetLayout()},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline,
            VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, true
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        return success;
    }

    bool UserInterface::SetupVertexBuffer()
    {
        struct SHADER_INPUT {
            Maths::Vector2 position;
            Maths::Vector2 uv;
        };

        std::vector<SHADER_INPUT> vertex_data(4);
        vertex_data[0].position = {-1.0f, -1.0f};
        vertex_data[0].uv = { 0.0f, 0.0f };
        vertex_data[1].position = {1.0f, -1.0f};
        vertex_data[1].uv = { 1.0f, 0.0f };
        vertex_data[2].position = {-1.0f, 1.0f};
        vertex_data[2].uv = { 0.0f, 1.0f };
        vertex_data[3].position = {1.0f, 1.0f};
        vertex_data[3].uv = { 1.0f, 1.0f };

        GlobalData::GetInstance()->instanced_buffer.WriteData(
            vertex_data.data(),
            static_cast<uint32_t>(vertex_data.size()) * sizeof(SHADER_INPUT),
            this->ui_vbo_chunk->offset
        );

        return true;
    }

    VkCommandBuffer UserInterface::BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        if(!this->refresh[frame_index]) return command_buffer;
        this->refresh[frame_index] = false;

        /*if(!this->need_update[frame_index]) return command_buffer;
        this->UpdateVertexBuffer(frame_index);
        this->need_update[frame_index] = false;*/

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::GetCommandBuffer() => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        VkClearValue clear_values[2];
        clear_values[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = this->render_pass;
        render_pass_begin_info.framebuffer = framebuffer;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.renderArea.extent.width = Vulkan::GetDrawSurface().width;
        render_pass_begin_info.renderArea.extent.height = Vulkan::GetDrawSurface().height;
        render_pass_begin_info.clearValueCount = 2;
        render_pass_begin_info.pClearValues = clear_values;

        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

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

        VkDescriptorSet set = GlobalData::GetInstance()->mouse_square_descriptor.Get(frame_index);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &set, 0, nullptr);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &GlobalData::GetInstance()->instanced_buffer.GetBuffer(frame_index).handle, &this->ui_vbo_chunk->offset);
        vkCmdDraw(command_buffer, 4, 1, 0, 0);

        vkCmdEndRenderPass(command_buffer);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::GetCommandBuffer() => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        return command_buffer;
    }

    void UserInterface::MouseMove(unsigned int x, unsigned int y)
    {
        if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_LEFT)) {
            this->selection_square.end = {x,y};
            this->update_selection_square = {true, true, true};
            if(this->selection_square.display == 0) {
                this->selection_square.display = 1;
                Camera::GetInstance()->Freeze();
            }
        }
    }

    void UserInterface::MouseButtonDown(MOUSE_BUTTON button)
    {
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT)
            this->selection_square.start = Mouse::GetInstance().GetPosition();
    }

    void UserInterface::MouseButtonUp(MOUSE_BUTTON button)
    {
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT) {
            if(this->selection_square.display > 0 && Camera::GetInstance()->IsRtsMode()) {
                this->selection_square.display = 0;
                this->update_selection_square = {true, true, true};
                Camera::GetInstance()->UnFreeze();
                for(auto& listener : this->Listeners)
                    listener->SquareSelection(this->selection_square.start, this->selection_square.end);
            }else{
                for(auto& listener : this->Listeners)
                    listener->ToggleSelection(Mouse::GetInstance().GetPosition());
            }
        }else if(button == MOUSE_BUTTON::MOUSE_BUTTON_RIGHT) {
            for(auto& listener : this->Listeners)
                listener->MoveToPosition(Mouse::GetInstance().GetPosition());
        }
    }
}