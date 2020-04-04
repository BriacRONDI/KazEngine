#include "Entity.h"
#include "../Core.h"

namespace Engine
{
    void Entity::Clear()
    {
        this->meshes.clear();
        this->matrix = IDENTITY_MATRIX;
        this->dynamic_buffer_offset = 0;
    }

    void Entity::MoveToPosition(Vector3 const& destination)
    {
        this->move_start = std::chrono::system_clock::now();
        this->move_initial_position = {this->matrix[12], this->matrix[13], this->matrix[14]};
        this->move_destination = destination;
        this->move_direction = this->move_destination - this->move_initial_position;
        this->move_length = this->move_direction.Length();
        this->move_direction = this->move_direction.Normalize();
        this->moving = true;

        float angle = std::atan(this->move_direction.z / this->move_direction.x);
        if(this->move_direction.x > 0.0f) this->matrix = Matrix4x4::RotationMatrix(angle - 90.0f * DEGREES_TO_RADIANS, {0.0, 1.0, 0.0}, false, false);
        else this->matrix = Matrix4x4::RotationMatrix(angle + 90.0f * DEGREES_TO_RADIANS, {0.0, 1.0, 0.0}, false, false);

        this->matrix[12] = this->move_initial_position.x;
        this->matrix[13] = this->move_initial_position.y;
        this->matrix[14] = this->move_initial_position.z;
    }

    void Entity::Update(ManagedBuffer& buffer)
    {
        if(this->moving) {
            this->move_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->move_start);
            Vector3 new_position = this->move_initial_position + this->move_direction * this->move_speed * static_cast<float>(this->move_duration.count());

            if((new_position - this->move_initial_position).Length() >= this->move_length) {
                this->moving = false;
                this->matrix[12] = this->move_destination.x;
                this->matrix[13] = this->move_destination.y;
                this->matrix[14] = this->move_destination.z;
            }else{
                this->matrix[12] = new_position.x;
                this->matrix[13] = new_position.y;
                this->matrix[14] = new_position.z;
            }
        }

        this->UpdateUBO(buffer);
    }

    uint32_t Entity::UpdateUBO(ManagedBuffer& buffer)
    {
        buffer.WriteData(&this->matrix, sizeof(Matrix4x4), this->dynamic_buffer_offset, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
        return sizeof(Matrix4x4);
    }

    std::vector<Mesh::HIT_BOX> Entity::GetHitBoxes()
    {
        std::vector<Mesh::HIT_BOX> hit_boxes;
        
        for(auto const& mesh : this->GetMeshes()) {
            Mesh::HIT_BOX entity_hit_box;
            entity_hit_box.far_right_top_point = this->matrix * mesh->hit_box.far_right_top_point;
            entity_hit_box.near_left_bottom_point = this->matrix * mesh->hit_box.near_left_bottom_point;
            hit_boxes.push_back(entity_hit_box);
        }

        return hit_boxes;
    }
}