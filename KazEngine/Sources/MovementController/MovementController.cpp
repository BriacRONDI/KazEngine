#include "MovementController.h"

namespace Engine
{
    bool MovementController::Initialize()
    {
        GlobalData::GetInstance()->group_descriptor.AddListener(this);
        UserInterface::GetInstance()->AddListener(this);

        this->group_count = reinterpret_cast<uint32_t*>(GlobalData::GetInstance()->group_descriptor.AccessData(0, GROUP_COUNT_BINDING));
        this->groups_array = reinterpret_cast<MOVEMENT_GROUP*>(GlobalData::GetInstance()->group_descriptor.AccessData(0, GROUP_DATA_BINDING));

        return true;
    }

    void MovementController::Clear()
    {
        GlobalData::GetInstance()->group_descriptor.RemoveListener(this);
        UserInterface::GetInstance()->RemoveListener(this);
    }

    void MovementController::MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding)
    {
        switch(binding) {

            case GROUP_COUNT_BINDING :
                this->group_count = reinterpret_cast<uint32_t*>(descriptor->AccessData(0, binding));
                break;

            case GROUP_DATA_BINDING :
                this->groups_array = reinterpret_cast<MOVEMENT_GROUP*>(descriptor->AccessData(0, binding));
                break;
        }
    }

    void MovementController::MoveToPosition(Point<uint32_t> mouse_position)
    {
        Area<float> const& near_plane_size = Camera::GetInstance()->GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -Camera::GetInstance()->GetUniformBuffer().position;
        Surface& draw_surface = Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        float x = static_cast<float>(mouse_position.X) - float_draw_surface.Width / 2.0f;
        float y = static_cast<float>(mouse_position.Y) - float_draw_surface.Height / 2.0f;

        x /= float_draw_surface.Width / 2.0f;
        y /= float_draw_surface.Height / 2.0f;

        Maths::Vector3 mouse_ray = Camera::GetInstance()->GetFrontVector() * Camera::GetInstance()->GetNearClipDistance()
                                 + Camera::GetInstance()->GetRightVector() * near_plane_size.Width * x
                                 - Camera::GetInstance()->GetUpVector() * near_plane_size.Height * y;

        mouse_ray = mouse_ray.Normalize();

        float ray_length;
        if(!Maths::intersect_plane({0, 1, 0}, {}, camera_position, mouse_ray, ray_length)) return;
        this->MoveUnits(camera_position + mouse_ray * ray_length);
    }

    void MovementController::MoveUnits(Maths::Vector3 destination)
    {
        MOVEMENT_GROUP group;
        group.destination = {destination.x, destination.z};
        group.scale = 2;
        group.unit_radius = 0.5f;
        group.unit_count = 0;
        group.inside_count = 0;
        group.fill_count = 0;
        group.padding = 0;

        uint32_t group_id = *this->group_count;
        for(uint8_t i=0; i<*this->group_count; i++) {
            MOVEMENT_GROUP group_check = this->groups_array[i];
            if(group_check.unit_count == 0) {
                group_id = i;
                break;
            }
        }

        for(auto& entity : DynamicEntityRenderer::GetInstance()->GetEntities()) {
            if(entity->selected && (entity->Matrix()[12] != destination.x || entity->Matrix()[14] != destination.z)) {
                group.unit_count++;
            }
        }

        if(group.unit_count == 0) return;

        std::vector<DynamicEntity*> moving_entities;
        for(auto& entity : DynamicEntityRenderer::GetInstance()->GetEntities()) {
            if(entity->selected && (entity->Matrix()[12] != destination.x || entity->Matrix()[14] != destination.z)) {
                if(entity->Movement().moving != -1) {
                    uint32_t gid = entity->Movement().moving >= 0 ? entity->Movement().moving : -entity->Movement().moving - 2;
                    if(this->groups_array[gid].unit_count > 0) this->groups_array[gid].unit_count--;
                }

                entity->Movement().destination = {destination.x, destination.z};
                entity->Movement().moving = group_id;
                moving_entities.push_back(entity);
            }
        }

        if(group_id == *this->group_count) {
            auto chunk = GlobalData::GetInstance()->group_descriptor.ReserveRange(sizeof(MOVEMENT_GROUP), GROUP_DATA_BINDING);
            if(chunk == nullptr) {
                #if defined(DISPLAY_LOGS)
                std::cout << "MovementControler::MoveUnits() : Not enough memory" << std::endl;
                #endif
                return;
            }

            (*this->group_count)++;
        }
        
        this->groups_array[group_id] = group;
    }

    void MovementController::Update()
    {
        for(uint8_t i=0; i<*this->group_count; i++) {
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
        }

        /*uint32_t count = *this->group_count - 1;
        for(uint8_t i=count; i>=0; i--) {
            MOVEMENT_GROUP& group_check = this->groups_array[i];
            if(group_check.unit_count = 0) (*this->group_count)--;
            else break;
        }*/

        GlobalData::GetInstance()->mapped_buffer.Flush();
    }
}