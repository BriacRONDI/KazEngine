#pragma once

#include <chrono>
#include <map>
#include <vector>
#include <string>
#include "../../../Maths/Sources/Maths.h"

namespace Model
{
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

            struct ANIMATION {
                std::string name;
                std::chrono::milliseconds duration;
            };

            uint32_t index;
            std::string name;
            Maths::Matrix4x4 transformation;
            std::map<std::string, ANIM_TRANSFORM> animations;   // Clé : nom de l'animation
            std::map<std::string, Maths::Matrix4x4> offsets;    // Clé : nom du mesh
            std::vector<Bone> children;

            Bone() : index(UINT32_MAX) {}
            std::vector<char> Serialize() const;
            uint32_t Deserialize(const char* data);
            uint32_t SerializedSize() const;
            uint32_t Count() const;

            // TODO : Écrire directement dans le UBO final plutôt que de passer par un buffer temporaire
            std::vector<ANIMATION> ListAnimations() const;
            void BuildUBO(std::vector<char>& skeleton, std::vector<char>& offsets, std::vector<char>& offsets_ids, std::map<std::string, std::pair<uint32_t, uint32_t>>& sub_segment_sizes, uint32_t alignment);
            void BuildBoneOffsetsSBO(std::vector<char>& offsets, std::vector<char>& offsets_ids, std::map<std::string, std::pair<uint32_t, uint32_t>>& dynamic_offsets, uint32_t alignment = 0);
            void BuildAnimationSBO(std::vector<char>& skeleton, std::string const& animation, uint32_t& frame_count,
                                   uint32_t& bone_per_frame, uint8_t frames_per_second = 30);
            std::chrono::milliseconds GetAnimationTotalDuration(std::string const& animation) const;

        private :

            void BuildSkeletonSBO(std::vector<char>& skeleton, Maths::Matrix4x4 const& parent_transformation, uint8_t max_count);
            void BuildSkeletonSBO(std::vector<char>& skeleton, Maths::Matrix4x4 const& parent_transformation, uint8_t max_count,
                                  std::chrono::milliseconds const& time, std::string const& animation, uint32_t base_offset);
            void BuildOffsetsSBO(std::vector<char>& offsets, std::vector<char>& offsets_ids, std::map<std::string, std::map<uint8_t, Maths::Matrix4x4*>> const& prepared_bone_offsets_ubo,
                                 std::map<std::string, std::pair<uint32_t, uint32_t>>& sub_segment_sizes, uint32_t alignment);
            void PrepareOffsetSBO(uint8_t max_count, std::map<std::string, std::map<uint8_t, Maths::Matrix4x4*>>& prepared_ubo);
            float EvalInterpolation(std::vector<KEYFRAME> const& keyframes, std::chrono::milliseconds const& time, KEYFRAME const& base = {});
    };
}