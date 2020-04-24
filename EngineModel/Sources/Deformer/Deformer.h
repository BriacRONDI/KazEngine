#pragma once

#include <array>
#include <vector>

namespace Model
{
    /**
     * A deformer is the association between a mesh vertex and some influencing bones.
     * A mesh vertex can be influenced up to 4 bones
     */
    class Deformer
    {
        public :
            static constexpr uint8_t MAX_BONES_PER_VERTEX  = 4;

            std::array<float, MAX_BONES_PER_VERTEX> bone_weights;
            std::array<uint32_t, MAX_BONES_PER_VERTEX> bone_ids;

            Deformer() : bone_ids({}), bone_weights({}) {}
            Deformer(Deformer const& other) : bone_ids(other.bone_ids), bone_weights(other.bone_weights) {}

            void AddBone(uint32_t id, float weight);
            void NormalizeWeights();
            inline uint8_t const Size() const { uint8_t size = 0; for(uint8_t i=0; i<this->bone_weights.size(); i++) if(this->bone_weights[i] != 0.0f) size++; return size; } 
            std::vector<char> Serialize();
            uint32_t Deserialize(const char* data);
    };
}