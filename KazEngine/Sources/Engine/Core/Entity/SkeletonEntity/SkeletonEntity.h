#pragma once

#include "../../Core.h"

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :

            uint16_t frame_index = 0;   // Indice de progression de l'animation
            std::string animation;      // Animation en cours

            virtual inline uint32_t UpdateUBO(ManagedBuffer& buffer)
            {
                uint32_t bytes_written = Entity::UpdateUBO(buffer);
                // buffer.WriteData(&this->bones, sizeof(std::array<Matrix4x4, MAX_BONES>), this->dynamic_buffer_offset + bytes_written, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
                return bytes_written;
            }

            virtual inline uint32_t GetUboSize()
            {
                return Entity::GetUboSize();
            }

            inline void InitializeBones(ModelManager const& model_manager)
            {
                /*for(auto const& mesh : this->meshes) {
                    if(mesh->name != "body") continue;
                    if(!mesh->skeleton.empty() && model_manager.skeletons.count(mesh->skeleton) > 0) {
                        this->InitializeBones(this->bones, *model_manager.skeletons.at(mesh->skeleton), mesh->name, {});
                    }
                }*/
            }

        private :

            /**
             * Positionne toutes les valeurs des bones d'une entité sur la transformation de repos
             */
            /*void InitializeBones(std::array<Matrix4x4, MAX_BONES>& ubo, BONE const& tree, std::string const& mesh, Matrix4x4 const& parent_transformation)
            {
                Matrix4x4 bone_transfromation = parent_transformation * tree.transformation;

                if(tree.offsets.count(mesh)) {
                    if(tree.offsets.count(mesh) && tree.index < ubo.size())
                        ubo[tree.index] = bone_transfromation * tree.offsets.at(mesh);
                }

                for(BONE const& child : tree.children) this->InitializeBones(ubo, child, mesh, bone_transfromation);
            }*/

            /*void EvalBones(UBO& ubo, BONE& tree, uint32_t vertex_buffer_id, Matrix4x4& parent_transformation, std::chrono::milliseconds& time, std::string const& animation)
            {
                Matrix4x4 bone_transfromation;
                if(tree.animation.count(animation)) {
                    Vector3 translation = this->EvalInterpolation(tree.animation[animation].translations, time);
                    Vector3 scaling = this->EvalInterpolation(tree.animation[animation].scalings, time, {std::chrono::milliseconds(0), {1,1,1}});
                    Vector3 rotation = this->EvalInterpolation(tree.animation[animation].rotations, time) * DEGREES_TO_RADIANS;
                    Matrix4x4 anim_transformation = TranslationMatrix(translation) * EulerRotation(IDENTITY_MATRIX, rotation, EULER_ORDER::ZYX) * ScalingMatrix(scaling);
                    bone_transfromation = parent_transformation * anim_transformation;
                }else{
                    bone_transfromation = parent_transformation * tree.transformation;
                }

                if(tree.offset.count(vertex_buffer_id) && tree.index < ubo.bones.size())
                    ubo.bones[tree.index] = bone_transfromation * tree.offset[vertex_buffer_id];

                for(BONE& child : tree.children) this->EvalBones(ubo, child, vertex_buffer_id, bone_transfromation, time, animation);
            }

            Vector3 EvalInterpolation(std::vector<VECTOR_KEYFRAME> const& keyframes, std::chrono::milliseconds const& time, VECTOR_KEYFRAME const& base)
            {
                if(keyframes.size() == 0) return base.value;

                uint32_t source_key = UINT32_MAX;
                uint32_t dest_key = UINT32_MAX;

                for(uint32_t i=0; i<keyframes.size(); i++) {
                    auto& keyframe = keyframes[i];
                    if(keyframe.time <= time) {
                        source_key = i;
                    } else {
                        dest_key = i;
                        break;
                    }
                }

                if(dest_key == UINT32_MAX) {

                    VECTOR_KEYFRAME const& last_keyframe = keyframes[keyframes.size() - 1];
                    return last_keyframe.value;

                }else{

                    VECTOR_KEYFRAME const& source_keyframe = (source_key == UINT32_MAX) ? base : keyframes[source_key];
                    VECTOR_KEYFRAME const& dest_keyframe = keyframes[dest_key];

                    std::chrono::milliseconds current_key_duration = dest_keyframe.time - source_keyframe.time;
                    std::chrono::milliseconds key_progression = time - source_keyframe.time;
                    float ratio = static_cast<float>(key_progression.count()) / static_cast<float>(current_key_duration.count());

                    return Interpolate(source_keyframe.value, dest_keyframe.value, ratio);
                }
            }

            void UpdateSkeleton(uint32_t mesh_id, std::chrono::milliseconds& time, std::string const& animation)
            {
                MESH& mesh = this->meshes[mesh_id];
                Matrix4x4 identity = IDENTITY_MATRIX;

                for(auto& resource : this->shared) {
                    ENTITY& entity = resource.entities[mesh_id];
                    this->EvalBones(entity.ubo, this->bone_trees[mesh.bone_tree_id], mesh.vertex_buffer_id, identity, time, animation);
                }
            }*/
    };
}