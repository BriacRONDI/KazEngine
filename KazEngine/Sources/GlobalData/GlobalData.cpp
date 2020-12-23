#include "GlobalData.h"
#include "../LOD/LOD.h"
#include "../DynamicEntity/DynamicEntity.h"
#include "../UserInterface/UserInterface.h"
#include "../MovementController/MovementController.h"

namespace Engine
{
    GlobalData::GlobalData()
    {
        this->instanced_buffer = InstancedBuffer(SIZE_MEGABYTE(64), INSTANCED_BUFFER_MASK, Vulkan::GetSwapChainImageCount(), {});
        this->mapped_buffer = MappedBuffer(SIZE_MEGABYTE((128+8)), MAPPED_BUFFER_MASK);

        // TEXTURE
        this->texture_descriptor.PrepareBindlessTexture(8);

        // SKELETON
        this->instance = this;
        this->skeleton_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, SIZE_MEGABYTE(5)},  // BONES
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, SIZE_KILOBYTE(1)},  // OFFSET IDS
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, SIZE_MEGABYTE(1)},  // OFFSETS
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, SIZE_KILOBYTE(1)},  // ANIMATIONS
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(TRIGGERED_ANIMATIONS)} // TRIGGERED ANIMATION IDS
        });

        // CAMERA
        this->camera_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(Maths::Matrix4x4) * 2 + sizeof(std::array<Maths::Vector4,6>) + sizeof(Maths::Vector4)}
        });

        // INDIRECT
        this->indirect_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(LODGroup::INDIRECT_COMMAND) * UNIT_PREALLOC_COUNT}
        });

        // LOD
        this->lod_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(LODGroup::LOD) * MAX_LOD_COUNT}
        });

        // DYNAMIC ENTITIES
        this->dynamic_entity_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, sizeof(Maths::Matrix4x4) * UNIT_PREALLOC_COUNT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(DynamicEntity::FRAME_DATA) * UNIT_PREALLOC_COUNT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(DynamicEntity::ANIMATION_DATA) * UNIT_PREALLOC_COUNT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, sizeof(DynamicEntity::MOVEMENT_DATA) * UNIT_PREALLOC_COUNT},
        });

        // TIME
        this->time_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t) * 2},
        });

        // MOUSE SQUARE
        this->mouse_square_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(UserInterface::MOUSE_SELECTION_SQUARE)}
        });

        // SELECTION
        this->selection_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t) * (UNIT_PREALLOC_COUNT + 1)},
        });

        // MOVEMENT
        this->group_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t)},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MovementController::MOVEMENT_GROUP)}
        });

        // GRID
        this->grid_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(GRID_METADATA)},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}
        });

        #if defined(DISPLAY_LOGS)
        this->debug_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(uint32_t) + sizeof(int32_t) }
        });
        uint32_t* debug_bone_depth = reinterpret_cast<uint32_t*>(GlobalData::GetInstance()->debug_descriptor.AccessData(0));
        *debug_bone_depth = 0;
        int32_t* debug_move_item = reinterpret_cast<int32_t*>(GlobalData::GetInstance()->debug_descriptor.AccessData(sizeof(uint32_t)));
        *debug_move_item = -1;
        #endif

        // VERTEX BUFFER
        this->vertex_buffer = this->instanced_buffer.GetChunk()->ReserveRange(0);
    }

    GlobalData::~GlobalData()
    {
        this->lod_descriptor.Clear();
        this->indirect_descriptor.Clear();
        this->camera_descriptor.Clear();
        this->skeleton_descriptor.Clear();
        this->texture_descriptor.Clear();
        this->time_descriptor.Clear();
        this->mouse_square_descriptor.Clear();
        this->selection_descriptor.Clear();
        this->group_descriptor.Clear();
        this->grid_descriptor.Clear();

        #if defined(DISPLAY_LOGS)
        this->debug_descriptor.Clear();
        #endif

        this->mapped_buffer.Clear();
        this->instanced_buffer.Clear();
    }
}