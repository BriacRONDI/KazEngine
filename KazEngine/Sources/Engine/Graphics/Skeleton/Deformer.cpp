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
        for(auto const& anim : this->animations) {
            serialized_size += static_cast<uint32_t>(sizeof(uint8_t) + anim.first.size() + sizeof(uint32_t) * 9);
            for(uint8_t i=0; i<3; i++) {
                serialized_size += static_cast<uint32_t>(anim.second.translations[i].size() * (sizeof(uint64_t) + sizeof(float)));
                serialized_size += static_cast<uint32_t>(anim.second.rotations[i].size() * (sizeof(uint64_t) + sizeof(float)));
                serialized_size += static_cast<uint32_t>(anim.second.scalings[i].size() * (sizeof(uint64_t) + sizeof(float)));
            }
        }

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
            for(uint8_t i=0; i<3; i++) {
                *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(animation.second.translations[i].size());
                offset += sizeof(uint32_t);
                for(auto const& keyframe : animation.second.translations[i]) {
                    *reinterpret_cast<uint64_t*>(output.data() + offset) = keyframe.time.count();
                    offset += sizeof(uint64_t);
                    *reinterpret_cast<float*>(output.data() + offset) = keyframe.value;
                    offset += sizeof(float);
                }
            }

            //Rotations
            for(uint8_t i=0; i<3; i++) {
                *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(animation.second.rotations[i].size());
                offset += sizeof(uint32_t);
                for(auto const& keyframe : animation.second.rotations[i]) {
                    *reinterpret_cast<uint64_t*>(output.data() + offset) = keyframe.time.count();
                    offset += sizeof(uint64_t);
                    *reinterpret_cast<float*>(output.data() + offset) = keyframe.value;
                    offset += sizeof(float);
                }
            }

            //Scalings
            for(uint8_t i=0; i<3; i++) {
                *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(animation.second.scalings[i].size());
                offset += sizeof(uint32_t);
                for(auto const& keyframe : animation.second.scalings[i]) {
                    *reinterpret_cast<uint64_t*>(output.data() + offset) = keyframe.time.count();
                    offset += sizeof(uint64_t);
                    *reinterpret_cast<float*>(output.data() + offset) = keyframe.value;
                    offset += sizeof(float);
                }
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
            for(uint8_t i=0; i<3; i++) {
                animation.translations[i].resize(*reinterpret_cast<const uint32_t*>(data + offset));
                offset += sizeof(uint32_t);
                for(auto& keyframe : animation.translations[i]) {
                    keyframe.time = std::chrono::milliseconds(*reinterpret_cast<const uint64_t*>(data + offset));
                    offset += sizeof(uint64_t);
                    keyframe.value = *reinterpret_cast<const float*>(data + offset);
                    offset += sizeof(float);
                }
            }

            // Rotations
            for(uint8_t i=0; i<3; i++) {
                animation.rotations[i].resize(*reinterpret_cast<const uint32_t*>(data + offset));
                offset += sizeof(uint32_t);
                for(auto& keyframe : animation.rotations[i]) {
                    keyframe.time = std::chrono::milliseconds(*reinterpret_cast<const uint64_t*>(data + offset));
                    offset += sizeof(uint64_t);
                    keyframe.value = *reinterpret_cast<const float*>(data + offset);
                    offset += sizeof(float);
                }
            }

            // Scalings
            for(uint8_t i=0; i<3; i++) {
                animation.scalings[i].resize(*reinterpret_cast<const uint32_t*>(data + offset));
                offset += sizeof(uint32_t);
                for(auto& keyframe : animation.scalings[i]) {
                    keyframe.time = std::chrono::milliseconds(*reinterpret_cast<const uint64_t*>(data + offset));
                    offset += sizeof(uint64_t);
                    keyframe.value = *reinterpret_cast<const float*>(data + offset);
                    offset += sizeof(float);
                }
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

    /**
     * Construction des Uniform Buffers qui seront passés à la carte graphique, un buffer contient le squelette, l'autre, les offsets des bones
     * @param skeleton[out] Buffer contenant le squelette
     * @param offsets[out] Buffer contenant les offsets des bones
     * @param mesh_ubo_offsets[out] Renvoie la liste des offsets des données associées à chaque mesh (ces données sont toutes contenus dans le buffer offsets)
     * @param alignment[in] Alignement mémoire des bone offsets (minUniformBufferOffsetAlignment)
     */
    void Bone::BuildUBO(std::vector<char>& skeleton, std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_ubo_offsets, uint32_t alignment)
    {
        this->BuildSkeletonSBO(skeleton, IDENTITY_MATRIX, Bone::MAX_BONES_PER_UBO);

        std::map<std::string, std::map<uint8_t, Matrix4x4*>> prepared_bone_offsets_ubo;
        this->PrepareOffsetUbo(static_cast<uint8_t>(skeleton.size() / sizeof(Matrix4x4)), prepared_bone_offsets_ubo);
        this->BuildOffsetsUBO(offsets, prepared_bone_offsets_ubo, mesh_ubo_offsets, alignment);
    }

    /**
     * Construction du Uniform Buffer des bone offsets qui sera passé à la carte graphique
     * @param offsets[out] Buffer contenant les offsets des bones
     * @param mesh_ubo_offsets[out] Renvoie la liste des offsets des données associées à chaque mesh (ces données sont toutes contenus dans le buffer offsets)
     * @param alignment[in] Alignement mémoire des bone offsets (minUniformBufferOffsetAlignment)
     */
    void Bone::BuildBoneOffsetsUBO(std::vector<char>& offsets, std::map<std::string, uint32_t>& mesh_ubo_offsets, uint32_t alignment)
    {
        std::map<std::string, std::map<uint8_t, Matrix4x4*>> prepared_bone_offsets_ubo;
        this->PrepareOffsetUbo(Bone::MAX_BONES_PER_UBO, prepared_bone_offsets_ubo);
        this->BuildOffsetsUBO(offsets, prepared_bone_offsets_ubo, mesh_ubo_offsets, alignment);
    }

    /**
     * Création d'un std::map indiquant pour chaque mesh, la liste des indices des bones auxquels il est associés ainsi que la valeur des matreices de ces bones
     * @param max_count[in] limite de bone offsets pour un mesh
     * @param prepared_ubo[out] Clé : nom du mesh, Valeur : std::map => Clé : indice du bone, Valeur bone offset du mesh
     */
    void Bone::PrepareOffsetUbo(uint8_t max_count, std::map<std::string, std::map<uint8_t, Matrix4x4*>>& prepared_ubo)
    {
        if(this->index < max_count && !this->offsets.empty())
            for(auto& offset : this->offsets)
                prepared_ubo[offset.first][this->index] = &offset.second;

        for(auto& child : this->children)
            child.PrepareOffsetUbo(max_count, prepared_ubo);
    }

    /**
     * Création du buffer des bone offsets, pour obtenir la transformation finale d'un mesh,
     * il faut multiplier chacun des bones auxquels il est associé à cette matrice
     * @param offsets[out] Buffer contenant les offsets des bones
     * @param prepared_bone_offsets_ubo[in] résultat de @ref PrepareOffsetUbo
     * @param mesh_ubo_offsets[out] Renvoie la liste des offsets des données associées à chaque mesh (ces données sont toutes contenus dans le buffer offsets)
     * @param alignment[in] Alignement mémoire des bone offsets (minUniformBufferOffsetAlignment)
     */
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

    /**
     * Création du squelette sans animation
     * @param skeleton[out] Buffer contenant le squelette
     * @param parent_transformation[in] Transformation du bone parent
     * @param max_count[in] limite du nombre de bones dansle squelette
     */
    void Bone::BuildSkeletonSBO(std::vector<char>& skeleton, Matrix4x4 const& parent_transformation, uint8_t max_count)
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
            child.BuildSkeletonSBO(skeleton, bone_transfromation, max_count);
    }

    /**
     * Création du squelette positionné à un instant donné d'une animation
     * @param skeleton[out] Buffer contenant le squelette
     * @param parent_transformation[in] Transformation du bone parent
     * @param max_count[in] limite du nombre de bones dansle squelette
     * @param time Avancement de l'animation dans le temps
     * @param animation Animation souhaitée
     */
    void Bone::BuildSkeletonSBO(std::vector<char>& skeleton, Matrix4x4 const& parent_transformation, uint8_t max_count,
                                std::chrono::milliseconds const& time, std::string const& animation, uint32_t base_offset)
    {
        // Évaluation de la transformation au temps indiqué
        Matrix4x4 bone_transfromation;
        if(this->animations.count(animation)) {
            Vector3 translation, scaling, rotation;
            for(uint8_t i=0; i<3; i++) {
                translation[i] = this->EvalInterpolation(this->animations[animation].translations[i], time);
                scaling[i] = this->EvalInterpolation(this->animations[animation].scalings[i], time, {std::chrono::milliseconds(0), 1.0f});
                rotation[i] = this->EvalInterpolation(this->animations[animation].rotations[i], time) * DEGREES_TO_RADIANS;
            }
            Matrix4x4 anim_transformation = Matrix4x4::TranslationMatrix(translation) * Matrix4x4::EulerRotation(IDENTITY_MATRIX, rotation, Matrix4x4::EULER_ORDER::ZYX) * Matrix4x4::ScalingMatrix(scaling);
            bone_transfromation = parent_transformation * anim_transformation;
        }else{
            bone_transfromation = parent_transformation * this->transformation;
        }

        // Écriture du bone
        if(this->index < max_count) {
            uint32_t skeleton_offset = base_offset + this->index * sizeof(Matrix4x4);
            if(skeleton.size() < skeleton_offset + sizeof(Matrix4x4)) skeleton.resize(skeleton_offset + sizeof(Matrix4x4));
            *reinterpret_cast<Matrix4x4*>(skeleton.data() + skeleton_offset) = bone_transfromation;
        }

        // Traitement récursif sur tous les enfants
        for(auto& child : this->children)
            child.BuildSkeletonSBO(skeleton, bone_transfromation, max_count, time, animation, base_offset);
    }

    /**
     * Calcule la durée totale de l'animation en cherchant la KeyFrame la plus avancée dans le temps
     * @param animation Nom de l'animation dont on souhaite connaitre la durée
     */
    std::chrono::milliseconds Bone::GetAnimationTotalDuration(std::string const& animation) const
    {
        std::chrono::milliseconds total_duration(0);
        for(auto const& item : this->animations) {
            if(item.first == animation) {
                for(uint8_t i=0; i<3; i++) {
                    for(auto const& translation : item.second.translations[i]) if(translation.time > total_duration) total_duration = translation.time;
                    for(auto const& rotation : item.second.rotations[i]) if(rotation.time > total_duration) total_duration = rotation.time;
                    for(auto const& scaling : item.second.scalings[i]) if(scaling.time > total_duration) total_duration = scaling.time;
                }
                break;
            }
        }

        for(auto const& child : this->children) {
            std::chrono::milliseconds child_total_duration = child.GetAnimationTotalDuration(animation);
            if(child_total_duration > total_duration) total_duration = child_total_duration;
        }

        return total_duration;
    }

    void Bone::BuildAnimationSBO(std::vector<char>& skeleton, std::string const& animation, uint16_t& frame_count, uint32_t& bone_per_frame, uint8_t frames_per_second)
    {
        std::chrono::milliseconds total_duration = this->GetAnimationTotalDuration(animation);
        std::chrono::milliseconds time_per_frame(1000 / frames_per_second);
        std::chrono::milliseconds final_duration = total_duration;
        if(final_duration.count() % time_per_frame.count() != 0)
            final_duration =  final_duration + time_per_frame - std::chrono::milliseconds(final_duration.count() % time_per_frame.count());

        this->BuildSkeletonSBO(skeleton, IDENTITY_MATRIX, Bone::MAX_BONES_PER_UBO, std::chrono::milliseconds(0), animation, 0);
        frame_count = static_cast<uint16_t>(final_duration /  time_per_frame);
        bone_per_frame = static_cast<uint32_t>(skeleton.size() / sizeof(Matrix4x4));
        for(uint16_t i=1; i<frame_count; i++)
            this->BuildSkeletonSBO(skeleton, IDENTITY_MATRIX, Bone::MAX_BONES_PER_UBO, time_per_frame * i, animation, static_cast<uint32_t>(skeleton.size()));
    }

    /**
     * Revoie la liste des animations d'un squelette avec leur durée
     */
    std::vector<Bone::ANIMATION> Bone::ListAnimations() const
    {
        std::vector<Bone::ANIMATION> animation_list;

        for(auto const& animation : this->animations) {
            std::chrono::milliseconds total_duration(0);
            for(uint8_t i=0; i<3; i++) {
                for(auto const& translation : animation.second.translations[i]) if(translation.time > total_duration) total_duration = translation.time;
                for(auto const& rotation : animation.second.rotations[i]) if(rotation.time > total_duration) total_duration = rotation.time;
                for(auto const& scaling : animation.second.scalings[i]) if(scaling.time > total_duration) total_duration = scaling.time;
            }

            animation_list.push_back({animation.first, total_duration});
        }

        for(auto const& child : this->children) {
            auto child_animations = child.ListAnimations();
            for(auto const& child_animation : child_animations) {
                bool animation_found = false;
                for(auto& animation : animation_list) {
                    if(child_animation.name == animation.name) {
                        if(child_animation.duration > animation.duration)
                            animation.duration = child_animation.duration;
                        animation_found = true;
                        break;
                    }
                }
                if(!animation_found) animation_list.push_back(child_animation);
            }
        }

        return animation_list;
    }

    /**
     * Interpolation d'une KeyFrame : Calcul d'un état intermédiaire entre une KeyFrame et la suivante afin de créer une animation sans discontinuité.
     * @param keyframes Liste des KeyFrames composant un mouvement
     * @param time Stade d'avancement de l'animation
     * @param base Stade initial de l'animation
     */
    float Bone::EvalInterpolation(std::vector<KEYFRAME> const& keyframes, std::chrono::milliseconds const& time, KEYFRAME const& base)
    {
        if(keyframes.size() == 0) return base.value;
        if(keyframes.size() == 1) return keyframes[0].value;
        if(time > keyframes[keyframes.size() - 1].time) return keyframes[keyframes.size() - 1].value;

        uint32_t source_key = UINT32_MAX;
        uint32_t dest_key = UINT32_MAX;

        // TODO : Amélioration => Recherche dichotomique
        for(uint32_t i=0; i<keyframes.size(); i++) {
            auto& keyframe = keyframes[i];
            if(keyframe.time <= time) {
                source_key = i;
            } else {
                dest_key = i;
                break;
            }
        }

        KEYFRAME const& source_keyframe = (source_key == UINT32_MAX) ? base : keyframes[source_key];
        KEYFRAME const& dest_keyframe = keyframes[dest_key];

        std::chrono::milliseconds current_key_duration = dest_keyframe.time - source_keyframe.time;
        std::chrono::milliseconds key_progression = time - source_keyframe.time;
        float ratio = static_cast<float>(key_progression.count()) / static_cast<float>(current_key_duration.count());

        return Maths::Interpolate(source_keyframe.value, dest_keyframe.value, ratio);
    }
}