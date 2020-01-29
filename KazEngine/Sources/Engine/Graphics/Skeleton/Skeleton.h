#pragma once

#include <chrono>
#include <map>
#include "../../Common/Maths/Maths.h"
#include "../../Graphics/Mesh/Mesh.h"

namespace Engine
{
    struct KEYFRAME {
        std::chrono::milliseconds time;
        Vector3 value;

        KEYFRAME() : time(std::chrono::milliseconds()), value({}) {}
        KEYFRAME(std::chrono::milliseconds t, Vector3 v) : time(t), value(v) {}
        KEYFRAME(KEYFRAME const& key) : time(key.time), value(key.value) {}
        bool operator==(KEYFRAME const& other) const { return other.time == this->time && other.value == this->value; }
        void operator=(KEYFRAME const& other) { this->time = other.time; this->value = other.value; }
    };

    struct ANIM_TRANSFORM {
        std::vector<KEYFRAME> translations;
        std::vector<KEYFRAME> rotations;
        std::vector<KEYFRAME> scalings;
    };

    struct BONE {
        static constexpr uint8_t MAX_BONES_PER_UBO  = 100;

        uint32_t index;
        std::string name;
        Matrix4x4 transformation;
        std::map<std::string, ANIM_TRANSFORM> animations;    // Clé : nom de l'animation
        std::map<std::string, Matrix4x4> offsets;            // Clé : nom du mesh
        std::vector<BONE> children;

        BONE() : index(UINT32_MAX) {}
        std::vector<char> Serialize() const;
        uint32_t Deserialize(const char* data);
        uint32_t SerializedSize() const;
        uint32_t Count() const;
        void BuildUBO(std::vector<char>& skeleton, std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_indices);
        void BuildSkeletonUBO(std::vector<char>& skeleton, Matrix4x4 const& parent_transformation, uint8_t max_count);
        void BuildOffsetsUBO(std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_indices, uint8_t max_count);
    };

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