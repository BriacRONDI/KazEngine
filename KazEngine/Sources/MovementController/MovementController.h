#pragma once

#include <Singleton.hpp>
#include "../UserInterface/UserInterface.h"
#include "../GlobalData/GlobalData.h"

namespace Engine
{
    class MovementController : public Singleton<MovementController>, public IUserInteraction, public IMappedDescriptorListener
    {
        friend class Singleton<MovementController>;

        public :

            struct MOVEMENT_GROUP {
                Maths::Vector2 destination;
		        int scale;
		        float unit_radius;
                int unit_count;
                uint32_t fill_count;
                uint32_t inside_count;
                uint32_t padding;
            };

            bool Initialize();
            void Clear();
            uint32_t& GroupCount() { return *this->group_count; }
            MOVEMENT_GROUP& Group(uint32_t id) { return this->groups_array[id]; }
            bool HasNewGroup() { return this->new_group != UINT32_MAX; }
            uint32_t GetNewGroup() { uint32_t val = this->new_group; this->new_group = UINT32_MAX; return val; }

            ////////////////////////////////
            // IUserInteraction interface //
            ////////////////////////////////

            void SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end){};
            void ToggleSelection(Point<uint32_t> mouse_position){};
            void MoveToPosition(Point<uint32_t> mouse_position);

            /////////////////////////////////////////
            // IMappedDescriptorListener interface //
            /////////////////////////////////////////

            void MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding);

        private :

            uint32_t* group_count;
            MOVEMENT_GROUP* groups_array;
            uint32_t new_group;

            MovementController(){};
            ~MovementController(){};
            void MoveUnits(Maths::Vector3 destination);
    };
}