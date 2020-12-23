#include "Core.h"

namespace Engine
{
    Core::Core()
    {
        this->command_pool = nullptr;
    }

    Core::~Core()
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());

        // Compute Shader
        this->init_group_shader.Clear();
        this->collision_shader.Clear();
        this->movement_shader.Clear();
        this->cull_lod_shader.Clear();

        // Movement Controller
        MovementController::GetInstance()->DestroyInstance();

        // Map
        Map::GetInstance()->DestroyInstance();

        // User Interface
        UserInterface::GetInstance()->DestroyInstance();

        // Compute Shader Semaphores
        for(auto semaphore : this->compute_semaphores_1) vk::Destroy(semaphore);
        for(auto semaphore : this->compute_semaphores_2) vk::Destroy(semaphore);
        for(auto semaphore : this->compute_semaphores_3) vk::Destroy(semaphore);

        // Entity Renderer
        DynamicEntityRenderer::GetInstance()->DestroyInstance();

        // Camera
        Camera::GetInstance()->DestroyInstance();

        // Global Data
        GlobalData::GetInstance()->DestroyInstance();

        // Draw resources
        for(auto resource : this->resources) {
            vk::Destroy(resource.fence);
            vk::Destroy(resource.draw_semaphore);
        }

        // Present Semaphores
        for(auto semaphore : this->present_semaphores) vk::Destroy(semaphore);

        // Command Pool
        vk::Destroy(this->command_pool);
    }

    bool Core::Initialize()
    {
        this->resources.resize(Vulkan::GetSwapChainImageCount());
        this->present_semaphores.resize(Vulkan::GetSwapChainImageCount());
        this->compute_semaphores_1.resize(Vulkan::GetSwapChainImageCount());
        this->compute_semaphores_2.resize(Vulkan::GetSwapChainImageCount());
        this->compute_semaphores_3.resize(Vulkan::GetSwapChainImageCount());

        if(!vk::CreateCommandPool(this->command_pool, Vulkan::GetGraphicsQueue().index)) return false;

        for(auto& resource : this->resources) {
            if(!vk::CreateCommandBuffer(this->command_pool, resource.command_buffer)) return false;
            if(!vk::CreateFence(resource.fence, false)) return false;
            if(!vk::CreateSemaphore(resource.draw_semaphore)) return false;
        }

        for(auto& semaphore : this->present_semaphores) {
            if(!vk::CreateSemaphore(semaphore)) return false;
        }

        for(auto& semaphore : this->compute_semaphores_1) if(!vk::CreateSemaphore(semaphore)) return false;
        for(auto& semaphore : this->compute_semaphores_2) if(!vk::CreateSemaphore(semaphore)) return false;
        for(auto& semaphore : this->compute_semaphores_3) if(!vk::CreateSemaphore(semaphore)) return false;

        GlobalData::CreateInstance();
        GlobalData::GetInstance()->dynamic_entity_descriptor.AddListener(this);
        GlobalData::GetInstance()->group_descriptor.AddListener(this);
        GlobalData::GetInstance()->grid_descriptor.AddListener(this);

        Camera::CreateInstance();
        DynamicEntityRenderer::CreateInstance();
        MovementController::CreateInstance();
        UserInterface::CreateInstance();
        Map::CreateInstance();

        if(!UserInterface::GetInstance()->Initialize()) return false;
        UserInterface::GetInstance()->AddListener(this);

        if(!MovementController::GetInstance()->Initialize()) return false;
        if(!Map::GetInstance()->Initialize()) return false;

        if(!this->cull_lod_shader.Load("./Shaders/cull_lod_anim.comp.spv", {
            GlobalData::GetInstance()->camera_descriptor.GetLayout(),
            GlobalData::GetInstance()->dynamic_entity_descriptor.GetLayout(),
            GlobalData::GetInstance()->indirect_descriptor.GetLayout(),
            GlobalData::GetInstance()->lod_descriptor.GetLayout(),
            GlobalData::GetInstance()->time_descriptor.GetLayout(),
            GlobalData::GetInstance()->grid_descriptor.GetLayout()
        })) return false;

        if(!this->movement_shader.Load("./Shaders/move_groups.comp.spv", {
            GlobalData::GetInstance()->dynamic_entity_descriptor.GetLayout(),
            GlobalData::GetInstance()->group_descriptor.GetLayout(),
            GlobalData::GetInstance()->time_descriptor.GetLayout(),
            GlobalData::GetInstance()->skeleton_descriptor.GetLayout()
        })) return false;

        if(!this->collision_shader.Load("./Shaders/move_collision.comp.spv", {
            GlobalData::GetInstance()->dynamic_entity_descriptor.GetLayout(),
            GlobalData::GetInstance()->time_descriptor.GetLayout(),
            GlobalData::GetInstance()->grid_descriptor.GetLayout()
        })) return false;

        VkPushConstantRange group_destination;
        group_destination.offset = 0;
        group_destination.size = sizeof(uint32_t);
        group_destination.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        if(!this->init_group_shader.Load("./Shaders/init_group.comp.spv", {
            GlobalData::GetInstance()->dynamic_entity_descriptor.GetLayout(),
            GlobalData::GetInstance()->group_descriptor.GetLayout(),
            GlobalData::GetInstance()->selection_descriptor.GetLayout(),
            GlobalData::GetInstance()->skeleton_descriptor.GetLayout(),
            GlobalData::GetInstance()->time_descriptor.GetLayout()
        }, {group_destination})) return false;

        return true;
    }

    void Core::MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding)
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());
        descriptor->Update(binding);
    }

    bool Core::AddToScene(DynamicEntity& entity)
    {
        return DynamicEntityRenderer::GetInstance()->AddToScene(entity);
    }

    bool Core::LoadSkeleton(Model::Bone skeleton, std::string idle, std::string move, std::string attack)
    {
        /////////////////////////
        //   Mesh offsets SBO  //
        // Mesh offets ids SBO //
        /////////////////////////

        // Output buffers
        std::vector<char> offsets_sbo;
        std::vector<char> offsets_ids;

        #if defined(DISPLAY_LOGS)
        std::cout << "EntityRender::LoadSkeleton() : BuildBoneOffsetsSBO" << std::endl;
        #endif

        // Build buffers
        std::map<std::string, std::pair<uint32_t, uint32_t>> dynamic_offsets;
        uint32_t sbo_alignment = static_cast<uint32_t>(Vulkan::GetDeviceLimits().minStorageBufferOffsetAlignment);
        skeleton.BuildBoneOffsetsSBO(offsets_sbo, offsets_ids, dynamic_offsets, sbo_alignment);

        // Write to GPU memory
        GlobalData::GetInstance()->skeleton_descriptor.WriteData(offsets_sbo.data(), offsets_sbo.size(), 0, SKELETON_OFFSETS_BINDING);
        GlobalData::GetInstance()->skeleton_descriptor.WriteData(offsets_ids.data(), offsets_ids.size(), 0, SKELETON_OFFSET_IDS_BINDING);

        ////////////////////
        // Animations SBO //
        ////////////////////

        #if defined(DISPLAY_LOGS)
        std::cout << "EntityRender::LoadSkeleton() : BuildAnimationSBO" << std::endl;
        #endif

        auto const animations = skeleton.ListAnimations();
        uint32_t animation_offset = 0;
        uint32_t frame_id = 0;

        GlobalData::TRIGGERED_ANIMATIONS triggered_animations;

        for(int32_t i=0; i<animations.size(); i++) {

            GlobalData::BAKED_ANIMATION baked_animation;
            baked_animation.id = i;
            baked_animation.duration_ms = static_cast<uint32_t>(skeleton.GetAnimationTotalDuration(animations[i].name).count());

            // Build buffer
            std::vector<char> skeleton_sbo;
            uint32_t bone_count;
            skeleton.BuildAnimationSBO(skeleton_sbo, animations[i].name, baked_animation.frame_count, bone_count, 30);

            if(animations[i].name == idle) triggered_animations.idle = baked_animation;
            else if(animations[i].name == move) triggered_animations.move = baked_animation;
            else if(animations[i].name == attack) triggered_animations.attack = baked_animation;
            
            // Write the bone count before frame offsets
            if(!i) GlobalData::GetInstance()->skeleton_descriptor.WriteData(&bone_count, sizeof(uint32_t), 0, SKELETON_ANIM_OFFSETS_BINDING);

            // Write the frame offset
            GlobalData::GetInstance()->skeleton_descriptor.WriteData(&frame_id, sizeof(uint32_t), sizeof(uint32_t) * (i + 1), SKELETON_ANIM_OFFSETS_BINDING);

            // Write animation to GPU memory
            GlobalData::GetInstance()->skeleton_descriptor.WriteData(skeleton_sbo.data(), skeleton_sbo.size(), animation_offset, SKELETON_BONES_BINDING);

            frame_id += static_cast<uint32_t>(skeleton_sbo.size() / sizeof(Maths::Matrix4x4));
            animation_offset += static_cast<uint32_t>(skeleton_sbo.size());
            GlobalData::GetInstance()->animations[animations[i].name] = baked_animation;
        }

        auto chunk = GlobalData::GetInstance()->skeleton_descriptor.ReserveRange(sizeof(GlobalData::TRIGGERED_ANIMATIONS), SKELETON_TRIGGER_ANIM_BINDING);
        if(chunk == nullptr) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::LoadSkeleton() => Not enough memory" << std::endl;
            #endif
            return false;
        }
        GlobalData::GetInstance()->skeleton_descriptor.WriteData(&triggered_animations, sizeof(GlobalData::TRIGGERED_ANIMATIONS), chunk->offset, SKELETON_TRIGGER_ANIM_BINDING);

        // Success
        return true;
    }

    bool Core::LoadModel(LODGroup& lod)
    {
        std::string texture = lod.GetTexture();
        if(texture.empty()) return false;

        int32_t texture_id = GlobalData::GetInstance()->texture_descriptor.GetTextureID(texture);
        if(texture_id < 0) return false;

        lod.SetTextureID(texture_id);
        return lod.Build();
    }

    bool Core::BuildRenderPass(uint32_t frame_index)
    {
        VkCommandBuffer command_buffer = this->resources[frame_index].command_buffer;
        VkFramebuffer frame_buffer = Vulkan::GetFrameBuffer(frame_index);

        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::BuildRenderPass(" << frame_index << ") => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        std::array<VkClearValue, 2> clear_value = {};
        clear_value[0].color = { 0.1f, 0.2f, 0.3f, 0.0f };
        clear_value[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = Vulkan::GetRenderPass();
        render_pass_begin_info.framebuffer = frame_buffer;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.renderArea.extent.width = Vulkan::GetDrawSurface().width;
        render_pass_begin_info.renderArea.extent.height = Vulkan::GetDrawSurface().height;
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_value.size());
        render_pass_begin_info.pClearValues = clear_value.data();

        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        auto& surface = Vulkan::GetDrawSurface();
        if(surface.width > 0 && surface.height > 0) {
            VkCommandBuffer command_buffers[2] = {
                DynamicEntityRenderer::GetInstance()->BuildCommandBuffer(frame_index, frame_buffer),
                // Map::GetInstance()->BuildCommandBuffer(frame_index, frame_buffer)
            };
            vkCmdExecuteCommands(command_buffer, 1, command_buffers);
        }

        vkCmdEndRenderPass(command_buffer);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::BuildRenderPass(" << frame_index << ") => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    void Core::SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end)
    {
        Area<float> const& near_plane_size = Camera::GetInstance()->GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -Camera::GetInstance()->GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        float x1 = static_cast<float>(box_start.X) - float_draw_surface.Width / 2.0f;
        float x2 = static_cast<float>(box_end.X) - float_draw_surface.Width / 2.0f;

        float y1 = static_cast<float>(box_start.Y) - float_draw_surface.Height / 2.0f;
        float y2 = static_cast<float>(box_end.Y) - float_draw_surface.Height / 2.0f;

        x1 /= float_draw_surface.Width / 2.0f;
        x2 /= float_draw_surface.Width / 2.0f;
        y1 /= float_draw_surface.Height / 2.0f;
        y2 /= float_draw_surface.Height / 2.0f;

        float left_x = std::min<float>(x1, x2);
        float right_x = std::max<float>(x1, x2);
        float top_y = std::min<float>(y1, y2);
        float bottom_y = std::max<float>(y1, y2);

        Maths::Vector3 base_near = camera_position + Camera::GetInstance()->GetFrontVector() * Camera::GetInstance()->GetNearClipDistance();
        Maths::Vector3 base_with = Camera::GetInstance()->GetRightVector() * near_plane_size.Width;
        Maths::Vector3 base_height = Camera::GetInstance()->GetUpVector() * near_plane_size.Height;

        Maths::Vector3 top_left_position = base_near + base_with * left_x - base_height * top_y;
        Maths::Vector3 bottom_right_position = base_near + base_with * right_x - base_height * bottom_y;

        Maths::Vector3 top_left_ray = (top_left_position - camera_position).Normalize();
        Maths::Vector3 bottom_right_ray = (bottom_right_position - camera_position).Normalize();

        Maths::Plane left_plane = {top_left_position, top_left_ray.Cross(-Camera::GetInstance()->GetUpVector())};
        Maths::Plane right_plane = {bottom_right_position, bottom_right_ray.Cross(Camera::GetInstance()->GetUpVector())};
        Maths::Plane top_plane = {top_left_position, top_left_ray.Cross(-Camera::GetInstance()->GetRightVector())};
        Maths::Plane bottom_plane = {bottom_right_position, bottom_right_ray.Cross(Camera::GetInstance()->GetRightVector())};

        std::vector<DynamicEntity*> entities;
        for(auto entity : DynamicEntityRenderer::GetInstance()->GetEntities()) {
            if(entity->InSelectBox(left_plane, right_plane, top_plane, bottom_plane)) {
                entity->selected = true;
                entities.push_back(entity);
            }else{
                entity->selected = false;
            }
        }

        uint32_t count = static_cast<uint32_t>(entities.size());
        
        if(GlobalData::GetInstance()->selection_descriptor.GetChunk()->range < (count + 1) * sizeof(uint32_t)) {
            auto chunk = GlobalData::GetInstance()->selection_descriptor.ReserveRange((count + 1) * sizeof(uint32_t));
            if(chunk == nullptr) {
                count = static_cast<uint32_t>(GlobalData::GetInstance()->selection_descriptor.GetChunk()->range / sizeof(uint32_t)) - 1;
                #if defined(DISPLAY_LOGS)
                std::cout << "Core::SquareSelection() : Not engough memory" << std::endl;
                #endif
            }
        }

        GlobalData::GetInstance()->selection_descriptor.WriteData(&count, sizeof(uint32_t), 0);

        std::vector<uint32_t> selected_ids(count);
        for(uint32_t i=0; i<count; i++) selected_ids[i] = entities[i]->InstanceId();

        GlobalData::GetInstance()->selection_descriptor.WriteData(selected_ids.data(), selected_ids.size() * sizeof(uint32_t), sizeof(uint32_t));
    }

    void Core::ToggleSelection(Point<uint32_t> mouse_position)
    {
        Area<float> const& near_plane_size = Camera::GetInstance()->GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -Camera::GetInstance()->GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        Point<float> real_mouse = {
            static_cast<float>(mouse_position.X) - float_draw_surface.Width / 2.0f,
            static_cast<float>(mouse_position.Y) - float_draw_surface.Height / 2.0f
        };

        real_mouse.X /= float_draw_surface.Width / 2.0f;
        real_mouse.Y /= float_draw_surface.Height / 2.0f;

        Maths::Vector3 mouse_world_position = camera_position + Camera::GetInstance()->GetFrontVector() * Camera::GetInstance()->GetNearClipDistance() + Camera::GetInstance()->GetRightVector()
                                            * near_plane_size.Width * real_mouse.X - Camera::GetInstance()->GetUpVector() * near_plane_size.Height * real_mouse.Y;
        Maths::Vector3 mouse_ray = mouse_world_position - camera_position;
        mouse_ray = mouse_ray.Normalize();

        DynamicEntity* selected_entity = nullptr;
        for(auto entity : DynamicEntityRenderer::GetInstance()->GetEntities()) {
            if(selected_entity != nullptr) {
                entity->selected = false;
            }else if(entity->IntersectRay(camera_position, mouse_ray)) {

                struct SELECTION_ID {
                    uint32_t count;
                    uint32_t id;
                };

                selected_entity = entity;
                entity->selected = true;
                SELECTION_ID selection = {1, entity->InstanceId()};
                GlobalData::GetInstance()->selection_descriptor.WriteData(&selection, sizeof(SELECTION_ID), 0);
            }else {
                entity->selected = false;
            }
        }

        if(selected_entity == nullptr) {
            uint32_t count = 0;
            GlobalData::GetInstance()->selection_descriptor.WriteData(&count, sizeof(uint32_t), 0);
        }
    }

    void Core::Loop()
    {
        static uint8_t semaphore_index = 0;
        uint32_t frame_index;

        if(!Vulkan::GetInstance()->AcquireNextImage(frame_index, this->present_semaphores[semaphore_index])) {
            DynamicEntityRenderer::GetInstance()->Refresh();
            Map::GetInstance()->Refresh();
            return ;
        }

        if(!vk::WaitForFence(this->resources[frame_index].fence)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::Loop() => vk::WaitForFence : Timeout" << std::endl;
            #endif
            return;
        }

        Timer::Update(frame_index);
        bool skeleton_updated = GlobalData::GetInstance()->skeleton_descriptor.Update(frame_index);
        bool indirect_updated = GlobalData::GetInstance()->indirect_descriptor.Update(frame_index);
        bool lod_updated = GlobalData::GetInstance()->lod_descriptor.Update(frame_index);

        if(skeleton_updated || indirect_updated || lod_updated) this->cull_lod_shader.Refresh(frame_index);

        Camera::GetInstance()->Update(frame_index);
        UserInterface::GetInstance()->Update(frame_index);
        GlobalData::GetInstance()->instanced_buffer.Flush(frame_index);
        GlobalData::GetInstance()->mapped_buffer.Flush();

        uint32_t lod_count = DynamicEntityRenderer::GetInstance()->GetLodCount();
        uint32_t group_count = MovementController::GetInstance()->GroupCount();
        uint32_t entity_count = static_cast<uint32_t>(DynamicEntityRenderer::GetInstance()->GetEntities().size());

        VkSemaphore wait_semaphore1;
        if(MovementController::GetInstance()->HasNewGroup()) {
            wait_semaphore1 = this->compute_semaphores_1[frame_index];

            uint32_t selection_count = *reinterpret_cast<uint32_t*>(GlobalData::GetInstance()->selection_descriptor.AccessData(0));
            std::array<uint32_t,3> shader_count = {selection_count, 1, 1};
            this->init_group_shader.Refresh(frame_index);

            std::vector<VkDescriptorSet> descriptor_sets = {
                GlobalData::GetInstance()->dynamic_entity_descriptor.Get(),
                GlobalData::GetInstance()->group_descriptor.Get(),
                GlobalData::GetInstance()->selection_descriptor.Get(),
                GlobalData::GetInstance()->skeleton_descriptor.Get(frame_index),
                GlobalData::GetInstance()->time_descriptor.Get(frame_index)
            };

            uint32_t group_id = MovementController::GetInstance()->GetNewGroup();
            VkCommandBuffer command_buffer = this->init_group_shader.BuildCommandBuffer(frame_index, descriptor_sets, shader_count, {&group_id});

            vk::SubmitQueue(
                {command_buffer},
                {this->present_semaphores[semaphore_index]},
                {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                {this->compute_semaphores_1[frame_index]},
                Vulkan::GetComputeQueue().handle,
                nullptr
            );

        }else{
            wait_semaphore1 = this->present_semaphores[semaphore_index];
        }

        VkSemaphore wait_semaphore2;
        if(lod_count > 0 || group_count > 0 || entity_count > 0) {
            
            std::vector<VkCommandBuffer> shader_command_buffers;
            wait_semaphore2 = this->compute_semaphores_2[frame_index];
            
            if(lod_count > 0) {
                std::array<uint32_t,3> cull_lod_shader_count = {lod_count, 1, 1};
                if(cull_lod_shader_count != this->cull_lod_shader.GetCount(frame_index)) this->cull_lod_shader.Refresh(frame_index);

                std::vector<VkDescriptorSet> cull_lod_descriptor_sets = {
                    GlobalData::GetInstance()->camera_descriptor.Get(frame_index),
                    GlobalData::GetInstance()->dynamic_entity_descriptor.Get(),
                    GlobalData::GetInstance()->indirect_descriptor.Get(frame_index),
                    GlobalData::GetInstance()->lod_descriptor.Get(frame_index),
                    GlobalData::GetInstance()->time_descriptor.Get(frame_index),
                    GlobalData::GetInstance()->grid_descriptor.Get()
                };
            
                shader_command_buffers.push_back(
                    this->cull_lod_shader.BuildCommandBuffer(frame_index, cull_lod_descriptor_sets, cull_lod_shader_count)
                );
            }

            if(group_count > 0 && entity_count > 0) {
                std::array<uint32_t,3> movement_shader_count = {entity_count, group_count, 1};
                if(movement_shader_count != this->movement_shader.GetCount(frame_index)) this->movement_shader.Refresh(frame_index);

                std::vector<VkDescriptorSet> movement_descriptor_sets = {
                    GlobalData::GetInstance()->dynamic_entity_descriptor.Get(),
                    GlobalData::GetInstance()->group_descriptor.Get(),
                    GlobalData::GetInstance()->time_descriptor.Get(frame_index),
                    GlobalData::GetInstance()->skeleton_descriptor.Get(frame_index)
                };

                shader_command_buffers.push_back(
                    this->movement_shader.BuildCommandBuffer(frame_index, movement_descriptor_sets, movement_shader_count)
                );
            }

            vk::SubmitQueue(
                shader_command_buffers,
                {wait_semaphore1},
                {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                {this->compute_semaphores_2[frame_index]},
                Vulkan::GetComputeQueue().handle,
                nullptr
            );
        }else{
            wait_semaphore2 = wait_semaphore1;
        }

        VkSemaphore wait_semaphore3 = wait_semaphore2;
        if(entity_count > 0) {
            std::array<uint32_t,3> collision_shader_count = {entity_count, 1, 1};
            if(collision_shader_count != this->collision_shader.GetCount(frame_index)) this->collision_shader.Refresh(frame_index);

            wait_semaphore3 = this->compute_semaphores_3[frame_index];

            std::vector<VkDescriptorSet> collision_descriptor_sets = {
                GlobalData::GetInstance()->dynamic_entity_descriptor.Get(),
                GlobalData::GetInstance()->time_descriptor.Get(frame_index),
                GlobalData::GetInstance()->grid_descriptor.Get()
            };

            VkCommandBuffer command_buffer = this->collision_shader.BuildCommandBuffer(frame_index, collision_descriptor_sets, collision_shader_count);

            vk::SubmitQueue(
                {command_buffer},
                {wait_semaphore2},
                {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                {this->compute_semaphores_3[frame_index]},
                Vulkan::GetComputeQueue().handle,
                nullptr
            );
        }

        this->BuildRenderPass(frame_index);
        std::vector<VkCommandBuffer> submit_command_buffers = {this->resources[frame_index].command_buffer};

        if(UserInterface::GetInstance()->DisplayInterface()) {
            VkCommandBuffer ui_command_buffer = UserInterface::GetInstance()->BuildCommandBuffer(frame_index, Vulkan::GetFrameBuffer(frame_index));
            submit_command_buffers.push_back(ui_command_buffer);
        }

        vk::SubmitQueue(
            submit_command_buffers,
            {wait_semaphore3},
            {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            {this->resources[frame_index].draw_semaphore},
            Vulkan::GetGraphicsQueue().handle,
            this->resources[frame_index].fence
        );

        if(!Vulkan::GetInstance()->PresentImage({this->resources[frame_index].draw_semaphore}, frame_index)) {
            DynamicEntityRenderer::GetInstance()->Refresh();
            Map::GetInstance()->Refresh();
        }

        semaphore_index = (semaphore_index + 1) % Vulkan::GetSwapChainImageCount();
    }
}