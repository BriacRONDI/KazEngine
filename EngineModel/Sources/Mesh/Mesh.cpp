#include "Mesh.h"

namespace Model
{
    Mesh::~Mesh()
    {
        #if defined(DISPLAY_LOGS)
        std::cout << "Mesh [" << this->name << "] : deleted" << std::endl;
        #endif
    }

    /**
     * Construit le VBO à envoyer sur la carte graphique
     * en fonction des éléments constituants le mesh
     */
    std::unique_ptr<char> Mesh::BuildVBO(size_t& output_size, size_t& index_buffer_offset)
    {
        //////////////////////////
        // Calcul de la taille  //
        // du buffer de sortie  //
        //////////////////////////

        // Buffer d'indices
        uint32_t index_buffer_size = 0;
        if(this->uv_index.empty() && !this->index_buffer.empty()) index_buffer_size = static_cast<uint32_t>(this->GetIndexBufferSize());

        // Vertex buffer
        uint32_t vertex_buffer_size;
        if(!this->uv_index.empty()) {
            // Autant de composantes qu'il n'y a d'indice
            vertex_buffer_size = static_cast<uint32_t>(this->index_buffer.size() * (sizeof(Maths::Vector3) + sizeof(Maths::Vector2)));
            if(!this->deformers.empty()) vertex_buffer_size += static_cast<uint32_t>(this->index_buffer.size() * sizeof(Deformer));
        }else{
            // Autant de composantes qu'il n'y a de sommets
            vertex_buffer_size = static_cast<uint32_t>(this->GetVertexBufferSize() + this->GetUvBufferSize());
            if(!this->deformers.empty()) vertex_buffer_size += static_cast<uint32_t>(this->vertex_buffer.size() * sizeof(Deformer));
        }

        // Taille finale
        output_size = vertex_buffer_size + index_buffer_size;

        // Création du buffer de sortie
        std::unique_ptr<char> output(new char[output_size]);
        uint32_t offset = 0;

        ///////////////////
        // Vertex Buffer //
        ///////////////////

        if(!this->uv_index.empty()) {
            // Si les UV ne sont pas appairés aux sommets, on va devoir associer chaque sommet à un UV
            // Au final, on trouvera un grand nombre de sommets dupliqués et on se passera d'index buffer,
            // mais c'est le meilleur compromis en termes de performances
            for(uint32_t i=0; i<this->index_buffer.size(); i++) {

                // Indice du sommet
                uint32_t vertex_index = this->index_buffer[i];

                // Position
                std::memcpy(output.get() + offset, &this->vertex_buffer[vertex_index], sizeof(Maths::Vector3));
                offset += sizeof(Maths::Vector3);

                // Normale
                /*if(!this->normal_buffer.empty()) {
                    std::memcpy(output.get() + offset, &this->normal_buffer[vertex_index], sizeof(Vector3));
                    offset += sizeof(Vector3);
                }*/

                // UV
                uint32_t uv_index = this->uv_index[i];
                std::memcpy(output.get() + offset, &this->uv_buffer[uv_index], sizeof(Maths::Vector2));
                offset += sizeof(Maths::Vector2);

                // Squelette
                if(this->deformers.size() > 1) {
                    std::memcpy(output.get() + offset, &this->deformers[vertex_index], sizeof(Deformer));
                    offset += sizeof(Deformer);
                }
            }

        }else{
            // Lorsque les UV son appairés aux sommets, on a une correspondance 1 à 1, ce qui est optimal
            for(uint32_t i=0; i<this->vertex_buffer.size(); i++) {

                // Position
                std::memcpy(output.get() + offset, &this->vertex_buffer[i], sizeof(Maths::Vector3));
                offset += sizeof(Maths::Vector3);

                // UV
                if(!this->uv_buffer.empty()) {
                    std::memcpy(output.get() + offset, &this->uv_buffer[i], sizeof(Maths::Vector2));
                    offset += sizeof(Maths::Vector2);
                }

                // Squelette
                if(this->deformers.size() > 1) {
                    std::memcpy(output.get() + offset, &this->deformers[i], sizeof(Deformer));
                    offset += sizeof(Deformer);
                }
            }
        }

        // Index Buffer
        if(index_buffer_size > 0) {
            index_buffer_offset = offset;
            std::memcpy(output.get() + offset, this->index_buffer.data(), index_buffer_size);
        }else{
            index_buffer_offset = UINT64_MAX;
        }

        // Fin
        return output;
    }

    /**
     * Sérialization
     */
    std::unique_ptr<char> Mesh::Serialize(uint32_t& output_size)
    {
        // Calcul de la taille du buffer de sortie
        SERIALIZED_HEADER header;
        header.name_length = static_cast<uint8_t>(this->name.size());
        header.skeleton_length = static_cast<uint8_t>(this->skeleton.size());
        header.vertex_count = static_cast<uint32_t>(this->vertex_buffer.size());
        header.index_count = static_cast<uint32_t>(this->index_buffer.size());
        header.uv_count = static_cast<uint32_t>(this->uv_buffer.size());
        header.uv_index_count = static_cast<uint32_t>(this->uv_index.size());
        header.deformer_count = static_cast<uint32_t>(this->deformers.size());
        header.normals = !this->normal_buffer.empty();
        header.texture_length = static_cast<uint8_t>(this->texture.size());

        uint32_t vertex_buffer_size = static_cast<uint32_t>(this->GetVertexBufferSize());
        uint32_t index_buffer_size = static_cast<uint32_t>(this->GetIndexBufferSize());
        uint32_t uv_buffer_size = static_cast<uint32_t>(this->GetUvBufferSize());
        uint32_t uv_index_buffer_size = static_cast<uint32_t>(this->GetUvIndexBufferSize());

        // La taille du buffer de déformeurs est variable
        uint32_t deformer_buffer_size = 0;
        for(auto& deformer : this->deformers)
            deformer_buffer_size += sizeof(uint8_t) + deformer.Size() * (sizeof(uint32_t) + sizeof(float));

        // Allocation du buffer de sérialization
        output_size = sizeof(header) + header.name_length + header.skeleton_length + vertex_buffer_size + index_buffer_size
                    + uv_buffer_size + uv_index_buffer_size + header.texture_length + deformer_buffer_size;
        if(header.normals) output_size += vertex_buffer_size;
        std::unique_ptr<char> output(new char[output_size]);

        // Insertion du header
        std::memcpy(output.get(), &header, sizeof(SERIALIZED_HEADER));

        // Insertion du nom
        size_t offset = sizeof(SERIALIZED_HEADER);
        std::memcpy(output.get() + offset, this->name.data(), header.name_length);
        offset += header.name_length;

        // Texture name
        std::memcpy(output.get() + offset, this->texture.data(), header.texture_length);
        offset += header.texture_length;

        if(!this->skeleton.empty()) {
            // Insertion du Nom du squelette rattaché au modèle
            std::memcpy(output.get() + offset, this->skeleton.data(), header.skeleton_length);
            offset += header.skeleton_length;
        }

        // Insertion du vertex buffer
        std::memcpy(output.get() + offset, this->vertex_buffer.data(), vertex_buffer_size);
        offset += vertex_buffer_size;

        if(!this->index_buffer.empty()) {
            // Insertion du index buffer
            std::memcpy(output.get() + offset, this->index_buffer.data(), index_buffer_size);
            offset += index_buffer_size;
        }

        if(!this->normal_buffer.empty()) {
            // Insertion du normal buffer
            std::memcpy(output.get() + offset, this->normal_buffer.data(), vertex_buffer_size);
            offset += vertex_buffer_size;
        }

        if(!this->uv_buffer.empty()) {
            // Insertion du uv buffer
            std::memcpy(output.get() + offset, this->uv_buffer.data(), uv_buffer_size);
            offset += uv_buffer_size;

            if(!this->uv_index.empty()) {
                // Insertion du uv index buffer
                std::memcpy(output.get() + offset, this->uv_index.data(), uv_index_buffer_size);
                offset += uv_index_buffer_size;
            }
        }

        if(!this->deformers.empty()) {
            // Insertion des déformeurs
            for(auto& deformer : this->deformers) {
                std::vector<char> serialized = deformer.Serialize();
                std::memcpy(output.get() + offset, serialized.data(), serialized.size());
                offset += serialized.size();
            }
        }

        // Renvoi du résultat
        return output;
    }

    void Mesh::MirrorY()
    {
        for(auto& vertex : this->vertex_buffer)
            vertex.y = -vertex.y;
    }

    /**
     * Désérialization
     */
    size_t Mesh::Deserialize(const char* data)
    {
        // Lecture du header
        uint32_t offset = 0;
        SERIALIZED_HEADER header;
        header = *reinterpret_cast<const SERIALIZED_HEADER*>(data + offset);
        offset += sizeof(SERIALIZED_HEADER);

        // Lecture du nom
        this->name = std::string(data + offset, data + offset + header.name_length);
        offset += header.name_length;

        // Texture
        this->texture = std::string(data + offset, data + offset + header.texture_length);
        offset += header.texture_length;

        if(header.skeleton_length) {
            // Lecture du squelette
            this->skeleton = std::string(data + offset, data + offset + header.skeleton_length);
            offset += header.skeleton_length;
        }

        // Lecture du vertex buffer
        this->vertex_buffer.resize(header.vertex_count);
        uint32_t vertex_buffer_size = static_cast<uint32_t>(this->GetVertexBufferSize());
        std::memcpy(this->vertex_buffer.data(), &data[offset], vertex_buffer_size);
        offset += vertex_buffer_size;

        if(header.index_count > 0) {
            // Lecture du index buffer
            this->index_buffer.resize(header.index_count);
            uint32_t index_buffer_size = static_cast<uint32_t>(this->GetIndexBufferSize());
            std::memcpy(this->index_buffer.data(), &data[offset], index_buffer_size);
            offset += index_buffer_size;
        }

        if(header.normals) {
            // Lecture du normal buffer
            this->normal_buffer.resize(header.vertex_count);
            std::memcpy(this->normal_buffer.data(), &data[offset], vertex_buffer_size);
            offset += vertex_buffer_size;
        }

        if(header.uv_count > 0) {
            // Lecture du uv buffer
            this->uv_buffer.resize(header.uv_count);
            uint32_t uv_buffer_size = static_cast<uint32_t>(this->GetUvBufferSize());
            std::memcpy(this->uv_buffer.data(), &data[offset], uv_buffer_size);
            offset += uv_buffer_size;

            if(header.uv_index_count > 0) {
                // Lecture du uv index buffer
                this->uv_index.resize(header.uv_index_count);
                uint32_t uv_index_buffer_size = static_cast<uint32_t>(this->GetUvIndexBufferSize());
                std::memcpy(this->uv_index.data(), &data[offset], uv_index_buffer_size);
                offset += uv_index_buffer_size;
            }
        }

        if(header.deformer_count > 0) {
            // Lecture des déformeurs
            this->deformers.resize(header.deformer_count);
            for(uint32_t i=0; i<header.deformer_count; i++)
                offset += this->deformers[i].Deserialize(data + offset);
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "Mesh [" << this->name << "] : Loaded" << std::endl;
        #endif

        // On renvoie le nombre d'octets lus
        return offset;
    }
}