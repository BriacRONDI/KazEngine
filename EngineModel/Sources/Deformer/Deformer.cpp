#include "Deformer.h"

namespace Model
{
    void Deformer::AddBone(uint32_t id, float weight)
    {
        for(uint8_t i=0; i<this->bone_weights.size(); i++) {
            if(this->bone_weights[i] == 0.0f) {
                this->bone_weights[i] = weight;
                this->bone_ids[i] = id;
                return;
            }
        }
    }

    void Deformer::NormalizeWeights()
    {
        float total_weight = 0.0f;    
        for(uint8_t i=0; i<this->bone_weights.size(); i++)
            total_weight += this->bone_weights[i];

        if(total_weight == 0.0f) return;

        if(total_weight != 1.0f)
            for(uint8_t i=0; i<this->bone_weights.size(); i++)
                this->bone_weights[i] = this->bone_weights[i] / total_weight;
    }

    std::vector<char> Deformer::Serialize()
    {
        // For safety, just normalize weights before serialization
        this->NormalizeWeights();

        // Count bones
        uint8_t size = this->Size();

        // If the deformer is empty, just leave
        if(!size) return {};

        // Buffer allocation
        std::vector<char> output(sizeof(uint8_t) + size * (sizeof(uint32_t) + sizeof(float)));

        // Writing components
        uint16_t offset = 0;
        output[offset] = size;
        offset++;

        // Bone IDs
        std::memcpy(&output[offset], this->bone_ids.data(), size * sizeof(uint32_t));
        offset += size * sizeof(uint32_t);

        // Bone Weights
        std::memcpy(&output[offset], this->bone_weights.data(), size * sizeof(float));

        // Success
        return output;
    }

    uint32_t Deformer::Deserialize(const char* data)
    {
        // Reading bone count
        uint16_t offset = 0;
        uint8_t size = data[offset];
        offset++;

        // Reading bone ids
        std::memcpy(this->bone_ids.data(), &data[offset], size * sizeof(uint32_t));
        offset += size * sizeof(uint32_t);

        // Reading bone weights
        std::memcpy(this->bone_weights.data(), &data[offset], size * sizeof(float));
        offset += size * sizeof(float);

        // Success
        return offset;
    }
}