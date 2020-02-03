#include "Skeleton.h"

namespace Engine
{
    /**
     * Renvoie la taille de l'objet après sérialisation
     */
    uint32_t Bone::SerializedSize() const
    {
        uint32_t serialized_size = static_cast<uint32_t>(sizeof(char) + this->name.size() + sizeof(uint32_t) + sizeof(Matrix4x4) + sizeof(uint16_t) * 3);

        // Taille des animations
        for(auto const& anim : this->animations)
            serialized_size += static_cast<uint32_t>(sizeof(uint8_t) + anim.first.size() + sizeof(uint32_t) * 3
                            + (anim.second.rotations.size() + anim.second.scalings.size() + anim.second.translations.size()) * (sizeof(uint64_t) + sizeof(Vector3)));

        // Taille des offsets
        for(auto const& offset : this->offsets)
            serialized_size += static_cast<uint32_t>(sizeof(uint8_t) + offset.first.size() + sizeof(Matrix4x4));

        // Taille des enfants
        for(auto const& child : this->children)
            serialized_size += child.SerializedSize();

        return serialized_size;
    }

    /**
     * Sérialisation
     */
    std::vector<char> Bone::Serialize() const
    {
        // Création du buffer de sortie
        std::vector<char> output(this->SerializedSize());

        // Insertion du nom
        uint8_t name_length = static_cast<uint8_t>(this->name.size());
        uint32_t offset = 0;
        output[offset] = name_length;
        offset++;
        std::memcpy(output.data() + offset, this->name.data(), this->name.size());
        offset += static_cast<uint32_t>(this->name.size());

        // Indice
        *reinterpret_cast<uint32_t*>(output.data() + offset) = this->index;
        offset += sizeof(uint32_t);

        // Insertion de la transformation
        *reinterpret_cast<Matrix4x4*>(output.data() + offset) = this->transformation;
        offset += sizeof(Matrix4x4);

        // Insertion des animations (keyframes)
        uint16_t animations_count = static_cast<uint16_t>(this->animations.size());
        *reinterpret_cast<uint16_t*>(output.data() + offset) = animations_count;
        offset += sizeof(uint16_t);

        for(auto& animation : this->animations) {

            // Nom de l'animation
            uint8_t animation_name_length = static_cast<uint8_t>(animation.first.size());
            output[offset] = animation_name_length;
            offset++;
            std::memcpy(output.data() + offset, animation.first.data(), animation.first.size());
            offset += static_cast<uint32_t>(animation.first.size());

            // Translations
            *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(animation.second.translations.size());
            offset += sizeof(uint32_t);
            for(auto const& keyframe : animation.second.translations) {
                *reinterpret_cast<uint64_t*>(output.data() + offset) = keyframe.time.count();
                offset += sizeof(uint64_t);
                *reinterpret_cast<Vector3*>(output.data() + offset) = keyframe.value;
                offset += sizeof(Vector3);
            }

            //Rotations
            *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(animation.second.rotations.size());
            offset += sizeof(uint32_t);
            for(auto const& keyframe : animation.second.rotations) {
                *reinterpret_cast<uint64_t*>(output.data() + offset) = keyframe.time.count();
                offset += sizeof(uint64_t);
                *reinterpret_cast<Vector3*>(output.data() + offset) = keyframe.value;
                offset += sizeof(Vector3);
            }

            //Scalings
            *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(animation.second.scalings.size());
            offset += sizeof(uint32_t);
            for(auto const& keyframe : animation.second.scalings) {
                *reinterpret_cast<uint64_t*>(output.data() + offset) = keyframe.time.count();
                offset += sizeof(uint64_t);
                *reinterpret_cast<Vector3*>(output.data() + offset) = keyframe.value;
                offset += sizeof(Vector3);
            }
        }

        // Insertion des offsets
        uint16_t offsets_count = static_cast<uint16_t>(this->offsets.size());
        *reinterpret_cast<uint16_t*>(output.data() + offset) = offsets_count;
        offset += sizeof(uint16_t);

        for(auto& offset_matrix : this->offsets) {

            // Nom de l'animation
            uint8_t offset_name_length = static_cast<uint8_t>(offset_matrix.first.size());
            output[offset] = offset_name_length;
            offset++;
            std::memcpy(output.data() + offset, offset_matrix.first.data(), offset_matrix.first.size());
            offset += static_cast<uint32_t>(offset_matrix.first.size());

            // Matrice
            *reinterpret_cast<Matrix4x4*>(output.data() + offset) = offset_matrix.second;
            offset += sizeof(Matrix4x4);
        }

        // Insertion des enfants
        uint16_t children_count = static_cast<uint16_t>(this->children.size());
        *reinterpret_cast<uint16_t*>(output.data() + offset) = children_count;
        offset += sizeof(uint16_t);

        for(auto const& child : this->children) {
            std::vector<char> serialized_child = child.Serialize();
            std::memcpy(output.data() + offset, serialized_child.data(), serialized_child.size());
            offset += static_cast<uint32_t>(serialized_child.size());
        }

        // Fin
        return output;
    }

    uint32_t Bone::Deserialize(const char* data)
    {
        uint32_t offset = 0;

        // Nom
        uint8_t name_length = data[offset];
        offset++;
        this->name = std::string(data + offset, data + offset + name_length);
        offset += name_length;

        // Indice
        this->index = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += sizeof(uint32_t);

        // Transformation
        this->transformation = *reinterpret_cast<const Matrix4x4*>(data + offset);
        offset += sizeof(Matrix4x4);

        // Animations (keyframes)
        uint16_t animations_count = *reinterpret_cast<const uint16_t*>(data + offset);
        offset += sizeof(uint16_t);

        for(uint16_t i=0; i<animations_count; i++) {
            ANIM_TRANSFORM animation;

            // Nom de l'animation
            uint8_t animation_name_length = data[offset];
            offset++;
            std::string animation_name = std::string(data + offset, data + offset + animation_name_length);
            offset += animation_name_length;

            // Translations
            uint32_t translations_count = *reinterpret_cast<const uint32_t*>(data + offset);
            offset += sizeof(uint32_t);
            for(uint32_t j=0; j<translations_count; j++) {
                KEYFRAME keyframe;
                keyframe.time = std::chrono::milliseconds(*reinterpret_cast<const uint64_t*>(data + offset));
                offset += sizeof(uint64_t);
                keyframe.value = *reinterpret_cast<const Vector3*>(data + offset);
                offset += sizeof(Vector3);
                animation.translations.push_back(keyframe);
            }

            // Rotations
            uint32_t rotations_count = *reinterpret_cast<const uint32_t*>(data + offset);
            offset += sizeof(uint32_t);
            for(uint32_t j=0; j<rotations_count; j++) {
                KEYFRAME keyframe;
                keyframe.time = std::chrono::milliseconds(*reinterpret_cast<const uint64_t*>(data + offset));
                offset += sizeof(uint64_t);
                keyframe.value = *reinterpret_cast<const Vector3*>(data + offset);
                offset += sizeof(Vector3);
                animation.rotations.push_back(keyframe);
            }

            // Scalings
            uint32_t scalings_count = *reinterpret_cast<const uint32_t*>(data + offset);
            offset += sizeof(uint32_t);
            for(uint32_t j=0; j<scalings_count; j++) {
                KEYFRAME keyframe;
                keyframe.time = std::chrono::milliseconds(*reinterpret_cast<const uint64_t*>(data + offset));
                offset += sizeof(uint64_t);
                keyframe.value = *reinterpret_cast<const Vector3*>(data + offset);
                offset += sizeof(Vector3);
                animation.scalings.push_back(keyframe);
            }

            this->animations[animation_name] = animation;
        }

        // Lecture des offsets
        uint16_t offsets_count = *reinterpret_cast<const uint16_t*>(data + offset);
        offset += sizeof(uint16_t);

        for(uint16_t i=0; i<offsets_count; i++) {
            // Nom de l'offset
            uint8_t offset_name_length = data[offset];
            offset++;
            std::string offset_name = std::string(data + offset, data + offset + offset_name_length);
            offset += offset_name_length;

            this->offsets[offset_name] = *reinterpret_cast<const Matrix4x4*>(data + offset);
            offset += sizeof(Matrix4x4);
        }

        // Insertion des enfants
        uint16_t child_count = *reinterpret_cast<const uint16_t*>(data + offset);
        offset += sizeof(uint16_t);
        this->children.resize(child_count);

        for(uint16_t i=0; i<child_count; i++)
            offset += this->children[i].Deserialize(data + offset);

        // On renvoie le nombre d'octets lus
        return offset;
    }

    void DEFORMER::AddBone(uint32_t id, float weight)
    {
        for(uint8_t i=0; i<this->bone_weights.size(); i++) {
            if(this->bone_weights[i] == 0.0f) {
                this->bone_weights[i] = weight;
                this->bone_ids[i] = id;
                return;
            }
        }
    }

    void DEFORMER::NormalizeWeights()
    {
        float total_weight = 0.0f;    
        for(uint8_t i=0; i<this->bone_weights.size(); i++)
            total_weight += this->bone_weights[i];

        if(total_weight != 1.0f)
            for(uint8_t i=0; i<this->bone_weights.size(); i++)
                this->bone_weights[i] = this->bone_weights[i] / total_weight;
    }

    /**
     * Sérialisation
     */
    std::vector<char> DEFORMER::Serialize()
    {
        // Par sécurité, on normalise toujours le déformeur avant de le sérialiser
        this->NormalizeWeights();

        // Comptage du nombre d'os contenus dans le deformer
        uint8_t size = this->Size();

        // Si le deformer est vide, on sort
        if(!size) return {};

        // Allocation du buffer
        std::vector<char> output(sizeof(uint8_t) + size * (sizeof(uint32_t) + sizeof(float)));

        // Écriture du nombre de composantes
        uint16_t offset = 0;
        output[offset] = size;
        offset++;

        // Bone IDs
        std::memcpy(&output[offset], this->bone_ids.data(), size * sizeof(uint32_t));
        offset += size * sizeof(uint32_t);

        // Bone Weights
        std::memcpy(&output[offset], this->bone_weights.data(), size * sizeof(float));

        // Succès
        return output;
    }

    uint32_t DEFORMER::Deserialize(const char* data)
    {
        // Lecture du nombre de composantes
        uint16_t offset = 0;
        uint8_t size = data[offset];
        offset++;

        // Bone IDs
        std::memcpy(this->bone_ids.data(), &data[offset], size * sizeof(uint32_t));
        offset += size * sizeof(uint32_t);

        // Bone Weights
        std::memcpy(this->bone_weights.data(), &data[offset], size * sizeof(float));
        offset += size * sizeof(float);

        // Succès
        return offset;
    }

    /**
     * Compte le nombre d'os contenus dans l'arbre
     */
    uint32_t Bone::Count() const
    {
        uint32_t count = 1;

        for(auto const& child : this->children)
            count += child.Count();

        return count;
    }

    void Bone::BuildUBO(std::vector<char>& skeleton, std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_ubo_offsets, uint32_t alignment)
    {
        this->BuildSkeletonUBO(skeleton, IDENTITY_MATRIX, Bone::MAX_BONES_PER_UBO);

        std::map<std::string, std::map<uint8_t, Matrix4x4*>> prepared_bone_offsets_ubo;
        this->PrepareOffsetUbo(static_cast<uint8_t>(skeleton.size() / sizeof(Matrix4x4)), prepared_bone_offsets_ubo);
        this->BuildOffsetsUBO(offsets, prepared_bone_offsets_ubo, mesh_ubo_offsets, alignment);
    }

    void Bone::PrepareOffsetUbo(uint8_t max_count, std::map<std::string, std::map<uint8_t, Matrix4x4*>>& prepared_ubo)
    {
        if(this->index < max_count && !this->offsets.empty())
            for(auto& offset : this->offsets)
                prepared_ubo[offset.first][this->index] = &offset.second;

        for(auto& child : this->children)
            child.PrepareOffsetUbo(max_count, prepared_ubo);
    }

    void Bone::BuildOffsetsUBO(std::vector<char>& offsets, std::map<std::string, std::map<uint8_t, Matrix4x4*>> const& prepared_bone_offsets_ubo,
                               std::map<std::string, uint32_t>& mesh_ubo_offsets, uint32_t alignment)
    {
        // Calcul de la taille du buffer final
        uint32_t output_size = 0;
        for(auto const& item : prepared_bone_offsets_ubo) {
            uint32_t buffer_size = static_cast<uint32_t>(sizeof(uint32_t) * 4 * Bone::MAX_BONES_PER_UBO + sizeof(Matrix4x4) * item.second.size());
            if(alignment > 0) buffer_size = static_cast<uint32_t>((buffer_size + alignment - 1) & ~(alignment - 1));
            output_size += buffer_size;
        }

        // Création du buffer de sortie
        offsets.resize(output_size);

        uint32_t write_offset = 0;
        for(auto const& item : prepared_bone_offsets_ubo) {
            uint32_t offset_index = 0;
            mesh_ubo_offsets[item.first] = write_offset;
            for(auto const& bone : item.second) {
                *reinterpret_cast<uint32_t*>(offsets.data() + write_offset + bone.first * sizeof(uint32_t) * 4) = offset_index;
                *reinterpret_cast<Matrix4x4*>(offsets.data() + write_offset + sizeof(uint32_t) * 4 * Bone::MAX_BONES_PER_UBO + offset_index * sizeof(Matrix4x4)) = *bone.second;
                offset_index++;
            }
            uint32_t buffer_size = static_cast<uint32_t>(sizeof(uint32_t) * 4 * Bone::MAX_BONES_PER_UBO + sizeof(Matrix4x4) * item.second.size());
            if(alignment > 0) buffer_size = static_cast<uint32_t>((buffer_size + alignment - 1) & ~(alignment - 1));
            write_offset += buffer_size;
        }
    }

    void Bone::BuildSkeletonUBO(std::vector<char>& skeleton, Matrix4x4 const& parent_transformation, uint8_t max_count)
    {
        // Calcul de la transformation globale
        Matrix4x4 bone_transfromation = parent_transformation * this->transformation;

        if(this->index < max_count) {

            // Écriture du bone
            uint32_t skeleton_offset = this->index * sizeof(Matrix4x4);
            if(skeleton.size() < skeleton_offset + sizeof(Matrix4x4)) skeleton.resize(skeleton_offset + sizeof(Matrix4x4));
            *reinterpret_cast<Matrix4x4*>(skeleton.data() + skeleton_offset) = bone_transfromation;
        }

        for(auto& child : this->children)
            child.BuildSkeletonUBO(skeleton, bone_transfromation, max_count);
    }
}