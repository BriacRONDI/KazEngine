#pragma once

#include <chrono>
#include <Singleton.hpp>
#include "../InstancedBuffer/InstancedBuffer.h"
#include "../MappedBuffer/MappedBuffer.h"
#include "../TextureDescriptor/TextureDescriptor.h"
#include "../InstancedDescriptorSet/InstancedDescriptorSet.h"
#include "../MappedDescriptorSet/MappedDescriptorSet.h"

#define UNIT_PREALLOC_COUNT 500000

#define SKELETON_BONES_BINDING          0
#define SKELETON_OFFSET_IDS_BINDING     1
#define SKELETON_OFFSETS_BINDING        2
#define SKELETON_ANIMATIONS_BINDING     3

#define ENTITY_MATRIX_BINDING           0
#define ENTITY_FRAME_BINDING            1
#define ENTITY_ANIMATION_BINDING        2
#define ENTITY_MOVEMENT_BINDING         3

#define GROUP_COUNT_BINDING             0
#define GROUP_DATA_BINDING              1

#define MAPPED_BUFFER_MASK VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
#define INSTANCED_BUFFER_MASK VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT

namespace Engine
{
    class GlobalData : public Singleton<GlobalData>
    {
        friend class Singleton<GlobalData>;

        public :

            struct BAKED_ANIMATION {
                uint32_t animation_id;
                uint32_t frame_count;
                std::chrono::milliseconds duration;
            };

            InstancedBuffer instanced_buffer;
            MappedBuffer mapped_buffer;

            TextureDescriptor texture_descriptor;
            InstancedDescriptorSet skeleton_descriptor;
            InstancedDescriptorSet indirect_descriptor;
            InstancedDescriptorSet camera_descriptor;
            InstancedDescriptorSet lod_descriptor;
            InstancedDescriptorSet time_descriptor;
            InstancedDescriptorSet mouse_square_descriptor;
            InstancedDescriptorSet selection_descriptor;
            MappedDescriptorSet dynamic_entity_descriptor;
            MappedDescriptorSet group_descriptor;
            std::shared_ptr<Chunk> vertex_buffer;
            std::map<std::string, BAKED_ANIMATION> animations;

        private :

            GlobalData();
            ~GlobalData();
    };
}