#pragma once

#include "../Entity/DynamicEntity/DynamicEntity.h"
#include "../Entity/IEntityListener.h"
#include "../ThreadedShader/IComputeResult.h"
#include "../Platform/Common/Timer/Timer.h"

#define DESCRIPTOR_BIND_MOVEMENT    static_cast<uint8_t>(0)
#define DESCRIPTOR_BIND_GROUP_COUNT static_cast<uint8_t>(1)
#define DESCRIPTOR_BIND_GROUPS      static_cast<uint8_t>(2)

namespace Engine
{
    class UnitControl : public IComputeResult, public IEntityListener
    {
        public :

            static bool Initialize();
            static inline UnitControl& Instance() { return *UnitControl::instance; }
            inline DescriptorSet const& GetMovementDescriptor() const { return this->movement_descriptor; }
            static void Clear();
            void MoveUnits(Maths::Vector3 destination);
            inline void Update() { this->move_groups_shader.Update(); this->move_collision_shader.Update(); }
            void Refresh();

            ////////////////////
            // IComputeResult //
            ////////////////////

            void ComputeFinished(ThreadedShader* thread);

            ////////////////////
            // IEntityListener //
            ////////////////////

            virtual void NewEntity(Entity* entity);
            virtual void AddLOD(Entity& entity, std::shared_ptr<LODGroup> lod) {};

        private :

            struct MOVEMENT_DATA {
                Maths::Vector2 destination;
                int moving;
                float radius;
                Maths::Vector4 debug;
            };

            struct MOVEMENT_GROUP {
                Maths::Vector2 destination;
		        int scale;
		        float unit_radius;
                uint32_t unit_count;
                uint32_t fill_count;
                uint32_t inside_count;
                uint32_t padding;
            };

            static UnitControl* instance;
            DescriptorSet movement_descriptor;
            MOVEMENT_GROUP* groups_array;
            MOVEMENT_DATA* movement_array;
            uint32_t* movement_group_count;
            ThreadedShader move_groups_shader;
            ThreadedShader move_collision_shader;

            void SetupCommandBuffers();
            void WriteMovement(VkCommandBuffer command_buffer, DynamicEntity* entity);
            UnitControl();
            ~UnitControl();
    };
}