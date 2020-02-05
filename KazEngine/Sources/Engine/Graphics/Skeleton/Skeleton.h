#pragma once

#include <chrono>
#include <map>
#include "../../Common/Maths/Maths.h"
#include "../../Graphics/Mesh/Mesh.h"

namespace Engine
{
    /*struct KEYFRAME {
        std::chrono::milliseconds time;
        Vector3 value;

        KEYFRAME() : time(std::chrono::milliseconds()), value({}) {}
        KEYFRAME(std::chrono::milliseconds t, Vector3 v) : time(t), value(v) {}
        KEYFRAME(KEYFRAME const& key) : time(key.time), value(key.value) {}
        bool operator==(KEYFRAME const& other) const { return other.time == this->time && other.value == this->value; }
        void operator=(KEYFRAME const& other) { this->time = other.time; this->value = other.value; }
    };*/

    /*struct KEYFRAME {

        enum COMPONENT : uint8_t {
            X   = 0b0001,
            Y   = 0b0010,
            Z   = 0b0100
        };
        
        std::chrono::milliseconds time;
        uint8_t components;
        std::vector<float> values;

        KEYFRAME() : components(0) {}
        inline bool operator==(KEYFRAME const& other) const { return other.time == this->time && other.components == this->components && other.values == this->values; }
        float GetComponent(COMPONENT c) const;
    };*/

    struct KEYFRAME {
        std::chrono::milliseconds time;
        float value;

        KEYFRAME() : time(std::chrono::milliseconds()), value(0.0f) {}
        KEYFRAME(std::chrono::milliseconds t, float v) : time(t), value(v) {}
        KEYFRAME(KEYFRAME const& other) : time(other.time), value(other.value) {}
        bool operator==(KEYFRAME const& other) const { return other.time == this->time && other.value == this->value; }
        void operator=(KEYFRAME const& other) { this->time = other.time; this->value = other.value; }
    };

    struct ANIM_TRANSFORM {
        std::array<std::vector<KEYFRAME>, 3> translations;
        std::array<std::vector<KEYFRAME>, 3> rotations;
        std::array<std::vector<KEYFRAME>, 3> scalings;
    };

    class Bone
    {
        public :
            static constexpr uint8_t MAX_BONES_PER_UBO  = 100;

            uint32_t index;
            std::string name;
            Matrix4x4 transformation;
            std::map<std::string, ANIM_TRANSFORM> animations;    // Clé : nom de l'animation
            std::map<std::string, Matrix4x4> offsets;            // Clé : nom du mesh
            std::vector<Bone> children;

            Bone() : index(UINT32_MAX) {}
            std::vector<char> Serialize() const;
            uint32_t Deserialize(const char* data);
            uint32_t SerializedSize() const;
            uint32_t Count() const;

            // TODO : Écrire directement dans le UBO final plutôt que de passer par un buffer temporaire
            void BuildUBO(std::vector<char>& skeleton, std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_indices, uint32_t alignment);
            void BuildBoneOffsetsUBO(std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_ubo_offsets, uint32_t alignment);
            void BuildAnimationSBO(std::vector<char>& skeleton, std::string const& animation, uint16_t& frame_count,
                                   uint32_t& bone_per_frame, uint8_t frames_per_second = 30);

        private :

            void BuildSkeletonSBO(std::vector<char>& skeleton, Matrix4x4 const& parent_transformation, uint8_t max_count);
            void BuildSkeletonSBO(std::vector<char>& skeleton, Matrix4x4 const& parent_transformation, uint8_t max_count,
                                  std::chrono::milliseconds const& time, std::string const& animation, uint32_t base_offset);
            void BuildOffsetsUBO(std::vector<char>& offsets, std::map<std::string, std::map<uint8_t, Matrix4x4*>> const& prepared_bone_offsets_ubo,
                                 std::map<std::string, uint32_t>& mesh_ubo_offsets, uint32_t alignment);
            void PrepareOffsetUbo(uint8_t max_count, std::map<std::string, std::map<uint8_t, Matrix4x4*>>& prepared_ubo);
            float EvalInterpolation(std::vector<KEYFRAME> const& keyframes, std::chrono::milliseconds const& time, KEYFRAME const& base = {});
            std::chrono::milliseconds GetAnimationTotalDuration(std::string const& animation) const;
    };

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

    struct DEFORMER {

        static constexpr uint8_t MAX_BONES_PER_VERTEX  = 4;

        std::array<float, MAX_BONES_PER_VERTEX> bone_weights;
        std::array<uint32_t, MAX_BONES_PER_VERTEX> bone_ids;

        DEFORMER() : bone_ids({}), bone_weights({}) {}
        DEFORMER(DEFORMER const& other) : bone_ids(other.bone_ids), bone_weights(other.bone_weights) {}

        void AddBone(uint32_t id, float weight);
        void NormalizeWeights();
        inline uint8_t const Size() const { uint8_t size = 0; for(uint8_t i=0; i<this->bone_weights.size(); i++) if(this->bone_weights[i] != 0.0f) size++; return size; } 
        std::vector<char> Serialize();
        uint32_t Deserialize(const char* data);
    };
}