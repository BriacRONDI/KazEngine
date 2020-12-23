#pragma once

#include <list>
#include <Maths.h>
#include "../LOD/LOD.h"
#include "../MappedDescriptorSet/IMappedDescriptorListener.h"
#include "../GlobalData/GlobalData.h"
#include "../Platform/Common/Timer/Timer.h"

namespace Engine
{
    class DynamicEntity : public IMappedDescriptorListener
    {
        public :

            struct FRAME_DATA {
                int32_t animation_id;
                uint32_t frame_id;
                FRAME_DATA() : animation_id(0), frame_id(0) {}
            };

            struct ANIMATION_DATA {
                uint32_t frame_count;
                uint32_t loop;
                uint32_t play;
                uint32_t duration;
                uint32_t start;
                float speed;
                ANIMATION_DATA() : frame_count(0), loop(0), play(0), duration(0), start(0), speed(0.0f) {}
            };

            class MOVEMENT_DATA {
                public :
                    Maths::Vector2 destination;
                    int32_t moving;
                    float radius;
                    std::array<int32_t, 4> grid_position;
                    int32_t skeleton_id;
                private :
	                std::array<int32_t, 3> padding = {};
            };

            bool selected;

            DynamicEntity();
            void AddModel(LODGroup* lod) { this->models.push_back(lod); }
            std::vector<LODGroup*> const GetModels() { return this->models; }
            uint32_t InstanceId() const { return this->instance_id; }
            void PlayAnimation(std::string animation, float speed, bool loop);
            void StopAnimation();
            bool InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane);
            bool IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction);

            Maths::Matrix4x4& Matrix() { return *this->matrix; }
            FRAME_DATA& Frame() { return *this->frame; }
            ANIMATION_DATA& Animation() { return *this->animation; }
            MOVEMENT_DATA& Movement() { return *this->movement; }

            // IDescriptorListener
            void MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding);

        private :

            friend class GlobalData;

            std::vector<LODGroup*> models;
            uint32_t instance_id;

            Maths::Matrix4x4* matrix;
            FRAME_DATA* frame;
            ANIMATION_DATA* animation;
            MOVEMENT_DATA* movement;
    };
}
