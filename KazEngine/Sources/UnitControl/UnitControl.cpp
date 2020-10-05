#include "UnitControl.h"

namespace Engine
{
    UnitControl* UnitControl::instance = nullptr;

    bool UnitControl::Initialize()
    {
        if(UnitControl::instance == nullptr) UnitControl::instance = new UnitControl;

        return true;
    }

    void UnitControl::Clear()
    {
        if(UnitControl::instance != nullptr) {
            delete UnitControl::instance;
            UnitControl::instance = nullptr;
        }
    }

    UnitControl::UnitControl()
    {
        if(!this->movement_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MOVEMENT_DATA) * 10},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t)},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MOVEMENT_GROUP) * 10}
        }, false)) return;

        this->movement_array = reinterpret_cast<MOVEMENT_DATA*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_MOVEMENT));
        this->movement_group_count = reinterpret_cast<uint32_t*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUP_COUNT));
        this->groups_array = reinterpret_cast<MOVEMENT_GROUP*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUPS));

        Entity::AddListener(this);

        this->move_groups_shader.AddListener(this);
        this->move_groups_shader.Init("move_groups.comp.spv", {&DynamicEntity::GetMatrixDescriptor(), &this->movement_descriptor, &Timer::GetDescriptor()}, false);

        this->move_collision_shader.AddListener(this);
        this->move_collision_shader.Init("move_collision.comp.spv", {&DynamicEntity::GetMatrixDescriptor(), &this->movement_descriptor, &Timer::GetDescriptor()}, false);
    }

    void UnitControl::Refresh()
    {
        uint32_t entity_count = static_cast<uint32_t>(DynamicEntity::GetEntities().size());

        this->move_groups_shader.Stop();
        if(*this->movement_group_count > 0 && entity_count > 0) {
            this->move_groups_shader.BuildCommandBuffer(entity_count, *this->movement_group_count, 1);
            this->move_groups_shader.Run(true);
        }

        this->move_collision_shader.Stop();
        if(entity_count > 1) {
            this->move_collision_shader.BuildCommandBuffer(entity_count, entity_count, 1);
            this->move_collision_shader.Run(true);
        }
    }

    UnitControl::~UnitControl()
    {
        this->move_groups_shader.Clear();
        this->move_collision_shader.Clear();
        this->movement_descriptor.Clear();
        Entity::RemoveListener(this);
    }

    void UnitControl::NewEntity(Entity* entity)
    {
        DynamicEntity* dynamic_entity = dynamic_cast<DynamicEntity*>(entity);
        if(dynamic_entity == nullptr) return;

        auto chunk = this->movement_descriptor.ReserveRange(sizeof(MOVEMENT_DATA));
        if(chunk == nullptr) {
            if(!this->movement_descriptor.ResizeChunk(this->movement_descriptor.GetChunk()->range + sizeof(MOVEMENT_DATA) * 10, 0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "UnitControl::GetFreeUnitChunk() : Not enough memory" << std::endl;
                #endif
                return;
            }else{
                chunk = this->movement_descriptor.ReserveRange(sizeof(MOVEMENT_DATA));
                this->movement_array = reinterpret_cast<MOVEMENT_DATA*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_MOVEMENT));
                this->movement_group_count = reinterpret_cast<uint32_t*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUP_COUNT));
                this->groups_array = reinterpret_cast<MOVEMENT_GROUP*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUPS));
            }
        }

        if(this->movement_descriptor.NeedUpdate(0)) {
            vkDeviceWaitIdle(Vulkan::GetDevice());
            this->movement_descriptor.Update(0);
        }

        MOVEMENT_DATA& movement = this->movement_array[chunk->offset / sizeof(MOVEMENT_DATA)];
        movement.destination = {};
        movement.moving = -1;
        movement.radius = 0.5f;

        dynamic_entity->SetMovementChunk(chunk);

        uint32_t entity_count = static_cast<uint32_t>(DynamicEntity::GetEntities().size());
        bool movement_updated = false;

        this->move_groups_shader.Stop();
        if(*this->movement_group_count > 0) {
            this->WriteMovement(this->move_groups_shader.GetWriteCommandBuffer(), dynamic_entity);
            this->move_groups_shader.UpdateWriteCommandBuffer();
            this->move_groups_shader.BuildCommandBuffer(entity_count, *this->movement_group_count, 1);
            this->move_groups_shader.Run(true);
            movement_updated = true;
        }

        this->move_collision_shader.Stop();
        if(entity_count > 1) {

            if(!movement_updated) {
                this->WriteMovement(this->move_collision_shader.GetWriteCommandBuffer(), dynamic_entity);
                this->move_collision_shader.UpdateWriteCommandBuffer();
                movement_updated = true;
            }

            this->move_collision_shader.BuildCommandBuffer(entity_count, entity_count, 1);
            this->move_collision_shader.Run(true);
        }

        if(!movement_updated) this->movement_descriptor.WriteData(&movement, sizeof(MOVEMENT_DATA), chunk->offset, DESCRIPTOR_BIND_MOVEMENT);
    }

    void UnitControl::WriteMovement(VkCommandBuffer command_buffer, DynamicEntity* entity)
    {
        auto& staging_buffer = DataBank::GetCommonBuffer().GetStagingBuffer(0);
        auto& data_buffer = DataBank::GetCommonBuffer().GetBuffer(0);
        // auto command_buffer = this->move_collision_shader.GetWriteCommandBuffer();

        VkMappedMemoryRange flush_range;
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);

        VkCommandBufferBeginInfo begin_info;
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr; 
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        std::vector<VkBufferCopy> copy_info;

        VkBufferCopy region;
        VkDeviceSize offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_MOVEMENT)->offset + entity->GetMovementChunk()->offset;
        region.srcOffset = offset;
        region.dstOffset = offset;
        region.size = sizeof(MOVEMENT_DATA);
        copy_info.push_back(region);

        vkCmdCopyBuffer(command_buffer, staging_buffer.handle, data_buffer.handle, static_cast<uint32_t>(copy_info.size()), copy_info.data());
        vkEndCommandBuffer(command_buffer);
    }

    void UnitControl::SetupCommandBuffers()
    {
        auto write_buffer = this->move_groups_shader.GetWriteCommandBuffer();
        auto read_buffer = this->move_groups_shader.GetReadCommandBuffer();
    }

    void UnitControl::MoveUnits(Maths::Vector3 destination)
    {
        MOVEMENT_GROUP group;
        group.destination = {destination.x, destination.z};
        group.scale = 2;
        group.unit_radius = 0.5f;
        group.unit_count = 0;
        group.inside_count = 0;
        group.fill_count = 0;
        group.padding = 0;

        uint32_t group_id = *this->movement_group_count;
        for(uint8_t i=0; i<*this->movement_group_count; i++) {
            MOVEMENT_GROUP group_check = this->groups_array[i];
            if(group_check.unit_count == 0) {
                group_id = i;
                break;
            }
        }

        for(auto& entity : DynamicEntity::GetEntities()) {
            if(entity->IsSelected() && (entity->GetMatrix()[12] != destination.x || entity->GetMatrix()[14] != destination.z)) {
                group.unit_count++;
            }
        }

        if(group.unit_count == 0) return;

        std::vector<DynamicEntity*> moving_entities;
        for(auto& entity : DynamicEntity::GetEntities()) {
            MOVEMENT_DATA& movement = this->movement_array[entity->GetMovementChunk()->offset / sizeof(MOVEMENT_DATA)];
            if(entity->IsSelected() && (entity->GetMatrix()[12] != destination.x || entity->GetMatrix()[14] != destination.z)) {
                if(movement.moving != -1) {
                    uint32_t gid = movement.moving >= 0 ? movement.moving : -movement.moving - 2;
                    if(this->groups_array[gid].unit_count > 0) this->groups_array[gid].unit_count--;
                }

                movement.destination = {destination.x, destination.z};
                movement.moving = group_id;
                moving_entities.push_back(entity);
            }
        }

        if(group_id == *this->movement_group_count) {
            auto chunk = this->movement_descriptor.ReserveRange(sizeof(MOVEMENT_GROUP), DESCRIPTOR_BIND_GROUPS);
            if(chunk == nullptr) {
                if(!this->movement_descriptor.ResizeChunk(
                    this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_GROUPS)->range + sizeof(MOVEMENT_GROUP),
                    DESCRIPTOR_BIND_GROUPS, Vulkan::SboAlignment()))
                {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "UnitControl::MoveUnits() : Not enough memory" << std::endl;
                    #endif
                    return;
                }else{
                    chunk = this->movement_descriptor.ReserveRange(sizeof(MOVEMENT_GROUP), DESCRIPTOR_BIND_GROUPS);
                    this->groups_array = reinterpret_cast<MOVEMENT_GROUP*>(this->movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUPS));
                }
            }

            (*this->movement_group_count)++;
        }
        
        this->groups_array[group_id] = group;

        this->move_groups_shader.Stop();

        ///////////////
        // WRITE JOB //
        ///////////////

        {
            auto& staging_buffer = DataBank::GetCommonBuffer().GetStagingBuffer(0);
            auto& data_buffer = DataBank::GetCommonBuffer().GetBuffer(0);
            auto command_buffer = this->move_groups_shader.GetWriteCommandBuffer();

            VkMappedMemoryRange flush_range;
            flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            flush_range.pNext = nullptr;
            flush_range.memory = staging_buffer.memory;
            flush_range.offset = 0;
            flush_range.size = VK_WHOLE_SIZE;
            vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);

            VkCommandBufferBeginInfo begin_info;
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.pNext = nullptr; 
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            begin_info.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(command_buffer, &begin_info);

            std::vector<VkBufferCopy> copy_info;

            {
                VkDeviceSize offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_GROUPS)->offset;
                VkBufferCopy region;
                region.srcOffset = offset;
                region.dstOffset = offset;
                region.size = *this->movement_group_count * sizeof(MOVEMENT_GROUP);
                copy_info.push_back(region);

                offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_GROUP_COUNT)->offset;
                region.srcOffset = offset;
                region.dstOffset = offset;
                region.size = sizeof(uint32_t);
                copy_info.push_back(region);
            }

            for(auto entity : moving_entities) {
                VkBufferCopy region;
                VkDeviceSize offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_MOVEMENT)->offset + entity->GetMovementChunk()->offset;
                region.srcOffset = offset;
                region.dstOffset = offset;
                region.size = sizeof(MOVEMENT_DATA);
                copy_info.push_back(region);
            }

            vkCmdCopyBuffer(command_buffer, staging_buffer.handle, data_buffer.handle, static_cast<uint32_t>(copy_info.size()), copy_info.data());
            vkEndCommandBuffer(command_buffer);
        }

        //////////////
        // READ JOB //
        //////////////

        if(*this->movement_group_count > 0) {
            auto& staging_buffer = DataBank::GetCommonBuffer().GetStagingBuffer(0);
            auto& data_buffer = DataBank::GetCommonBuffer().GetBuffer(0);
            auto command_buffer = this->move_groups_shader.GetReadCommandBuffer();

            VkCommandBufferBeginInfo begin_info;
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.pNext = nullptr; 
            begin_info.flags = 0;
            begin_info.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(command_buffer, &begin_info);

            std::vector<VkBufferCopy> copy_info;

            VkBufferCopy region;
            VkDeviceSize offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_GROUPS)->offset;
            region.srcOffset = offset;
            region.dstOffset = offset;
            region.size = sizeof(MOVEMENT_GROUP) * (*this->movement_group_count);
            copy_info.push_back(region);

            offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_MOVEMENT)->offset;
            region.srcOffset = offset;
            region.dstOffset = offset;
            region.size = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_MOVEMENT)->range;
            copy_info.push_back(region);

            vkCmdCopyBuffer(command_buffer, data_buffer.handle, staging_buffer.handle, static_cast<uint32_t>(copy_info.size()), copy_info.data());
            vkEndCommandBuffer(command_buffer);
        }

        ////////////////////////
        // RUN COMPUTE SHADER //
        ////////////////////////

        this->move_groups_shader.UpdateWriteCommandBuffer();
        this->move_groups_shader.UpdateReadCommandBuffer(*this->movement_group_count > 0);
        this->move_groups_shader.BuildCommandBuffer(static_cast<uint32_t>(DynamicEntity::GetEntities().size()), *this->movement_group_count, 1);
        this->move_groups_shader.Run(true);
    }

    void UnitControl::ComputeFinished(ThreadedShader* thread)
    {
        if(thread != &this->move_groups_shader) return;

        auto& staging_buffer = DataBank::GetCommonBuffer().GetStagingBuffer(0);
        auto& data_buffer = DataBank::GetCommonBuffer().GetBuffer(0);
        auto command_buffer = this->move_groups_shader.GetWriteCommandBuffer();

        VkMappedMemoryRange flush_range;
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);

        VkCommandBufferBeginInfo begin_info;
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr; 
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        std::vector<VkBufferCopy> copy_info;

        for(uint8_t i=0; i<*this->movement_group_count; i++) {
            MOVEMENT_GROUP& group_check = this->groups_array[i];

            if(group_check.inside_count >= group_check.unit_count) {
                group_check.unit_count = 0;
            }else{
                uint32_t max_count = 1;
		        for(int i=1; i<group_check.scale; i++) max_count += i * 6;
                if(group_check.inside_count >= max_count) group_check.scale++;
            }

            group_check.fill_count = 0;
            group_check.inside_count = 0;

            VkDeviceSize offset = this->movement_descriptor.GetChunk(DESCRIPTOR_BIND_GROUPS)->offset + i * sizeof(MOVEMENT_GROUP);
            VkBufferCopy region;
            region.srcOffset = offset;
            region.dstOffset = offset;
            region.size = sizeof(MOVEMENT_GROUP);
            copy_info.push_back(region);
        }

        vkCmdCopyBuffer(command_buffer, staging_buffer.handle, data_buffer.handle, static_cast<uint32_t>(copy_info.size()), copy_info.data());
        vkEndCommandBuffer(command_buffer);

        this->move_groups_shader.UpdateWriteCommandBuffer();
    }
}