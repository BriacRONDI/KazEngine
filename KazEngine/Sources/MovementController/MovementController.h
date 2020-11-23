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
                uint32_t unit_count;
                uint32_t fill_count;
                uint32_t inside_count;
                uint32_t padding;
            };

            bool Initialize();
            void Clear();
            uint32_t& GroupCount() { return *this->group_count; }
            void Update();

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

            MovementController(){};
            ~MovementController(){};
            void MoveUnits(Maths::Vector3 destination);
    };
}