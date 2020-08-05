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
     * Calcul le masque d�terminant la pipeline � utiliser pour afficher le mesh
     */
    void Mesh::UpdateRenderMask(std::map<std::string, MATERIAL> const& materials)
    {
        // Tous les mod�les ont des sommets
        this->render_mask = RENDER_MASK::RENDER_POSITION;

        // Normales
        // if(!this->normal_buffer.empty()) this->render_mask |= Renderer::NORMAL_VERTEX;

        // UV
        if(!this->uv_buffer.empty()) this->render_mask |= RENDER_MASK::RENDER_UV;

        // Mat�riaux
        // if(!this->materials.empty()) this->render_mask |= RENDER_MASK::RENDER_MATERIAL;

        // Squelette
        if(this->deformers.size() == 1) this->render_mask |= RENDER_MASK::RENDER_SINGLE_BONE;
        if(this->deformers.size() > 1) this->render_mask |= RENDER_MASK::RENDER_SKELETON;

        // Textures
        for(auto& material : this->materials) {
            if(materials.count(material.first) > 0 && !materials.at(material.first).texture.empty()) {

                // Cas sp�cial, ce mod�le dispose de texture mais sans UV,
                // dans ce cas on ignore la texture, seul le mat�riau sera pris en compte
                if(this->uv_buffer.empty()) return;

                this->render_mask |= RENDER_MASK::RENDER_TEXTURE;
                return;
            }
        }

        return;
    }

    /**
     * Construit le VBO � envoyer sur la carte graphique,
     * dans le cas d'un mesh d�coup� en sous-parties.
     */
    std::unique_ptr<char> Mesh::BuildSubMeshVBO(size_t& output_size, size_t& index_buffer_offset)
    {
        //////////////////////////
        // Calcul de la taille  //
        // du buffer de sortie  //
        //////////////////////////

        uint32_t vertex_buffer_size;
        uint32_t index_buffer_size = 0;
        uint32_t vertex_size = sizeof(Maths::Vector3);

        if(!this->uv_index.empty()) {
            // Autant de composantes qu'il y a d'indices
            vertex_size += sizeof(Maths::Vector2);
            vertex_buffer_size = static_cast<uint32_t>(this->index_buffer.size() * vertex_size);
        }else{
            // Autant de composantes qu'il y a de sommets
            vertex_buffer_size = static_cast<uint32_t>(this->vertex_buffer.size() * vertex_size);

            // Toutes les faces sont des triangles, donc pour chaque face, on a 3 sommet
            // soit un index buffer de taille 3 * nombre de faces
            for(auto& material : this->materials)
                index_buffer_size += static_cast<uint32_t>(material.second.size() * 3 * sizeof(uint32_t));
        }

        // Taille finale
        output_size = vertex_buffer_size + index_buffer_size;

        // Cr�ation du buffer de sortie
        std::unique_ptr<char> output(new char[output_size]);
        uint32_t offset = 0;

        ///////////////////
        // Vertex Buffer //
        ///////////////////

        if(!this->uv_index.empty()) {
            // Si les UV ne sont pas appair�s aux sommets, on va devoir associer chaque sommet � un UV.
            // Au final, on trouvera un grand nombre de sommets dupliqu�s et on se passera d'index buffer,
            // mais c'est le meilleur compromis en termes de performances
            for(uint32_t i=0; i<this->materials.size(); i++) {

                // Mat�riau correspondant au sous-model
                auto& material = this->materials[i];

                // D�claration du sous-model
                SUB_MESH sub_mesh;

                // Un sous-model est d�crit par ses faces, qui sont des trianges.
                // Le nombre de composantes d'un sous-model est donc le triple du nombre de faces
                sub_mesh.index_count = static_cast<uint32_t>(material.second.size() * 3);

                // Les sous-models sont contigus en m�moire, le premier commence � zero.
                // L'indice du sous-model correspond au nombre de composantes d�j� �crites,
                // soit le nombre d'octets divis� par la taille d'une composante.
                sub_mesh.first_index = offset / vertex_size;

                // On m�morise ces donn�es pour l'affichage
                this->sub_meshes.push_back(std::pair<std::string, SUB_MESH>(material.first, sub_mesh));

                // �criture des composantes du sous-model
                for(auto& face_index : material.second) {

                    for(uint8_t j=0; j<3; j++) {
                        // Position
                        uint32_t vertex_index = this->index_buffer[face_index * 3 + j];
                        std::memcpy(output.get() + offset, &this->vertex_buffer[vertex_index], sizeof(Maths::Vector3));
                        offset += sizeof(Maths::Vector3);

                        // UV
                        uint32_t uv_index = this->uv_index[face_index * 3 + j];
                        std::memcpy(output.get() + offset, &this->uv_buffer[uv_index], sizeof(Maths::Vector2));
                        offset += sizeof(Maths::Vector2);
                    }
                }
            }

        }else{
            for(uint32_t i=0; i<this->vertex_buffer.size(); i++) {

                // Position
                std::memcpy(output.get() + offset, &this->vertex_buffer[i], sizeof(Maths::Vector3));
                offset += sizeof(Maths::Vector3);

                // UV
                if(!this->uv_buffer.empty()) {
                    std::memcpy(output.get() + offset, &this->uv_buffer[i], sizeof(Maths::Vector2));
                    offset += sizeof(Maths::Vector2);
                }
            }

            // Un mesh d�coup� en sous-parties ne contiendra pas d'index buffer global,
            // Cependant, nous avons besoin de connaitre la position des index buffers des sous-parties
            // Chaque index buffer sera coll� l'un � l'autre formant un gros buffer dont on souhaite conserver l'offset
            index_buffer_offset = offset;

            // Index Buffer : Pour chaque mat�riau on aura un index buffer afin de s�parer le mesh en sous-parties
            // Chacune de ces sous-parties sera alors affich�e s�par�ment avec son mat�riau propre
            for(auto& material : this->materials) {

                // D�claration de la sous-partie du mesh
                SUB_MESH sub_mesh;
                sub_mesh.index_count = static_cast<uint32_t>(material.second.size() * 3);
                sub_mesh.first_index = static_cast<uint32_t>((offset - index_buffer_offset) / sizeof(uint32_t));
                this->sub_meshes.push_back(std::pair<std::string, SUB_MESH>(material.first, sub_mesh));
            
                for(auto& face_index : material.second) {
                    std::memcpy(output.get() + offset, &this->index_buffer[face_index * 3], sizeof(uint32_t) * 3);
                    offset += sizeof(uint32_t) * 3;
                }
            }
        }

        // Fin
        return output;
    }

    /**
     * Construit le VBO � envoyer sur la carte graphique
     * en fonction des �l�ments constituants le mesh
     */
    std::unique_ptr<char> Mesh::BuildVBO(size_t& output_size, size_t& index_buffer_offset)
    {
        // Si le mesh est divis� en sous-parties, on appelle BuildSubMeshVBO
        if(this->materials.size() > 1 && !this->materials[0].second.empty())
            return this->BuildSubMeshVBO(output_size, index_buffer_offset);

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

        // Cr�ation du buffer de sortie
        std::unique_ptr<char> output(new char[output_size]);
        uint32_t offset = 0;

        ///////////////////
        // Vertex Buffer //
        ///////////////////

        if(!this->uv_index.empty()) {
            // Si les UV ne sont pas appair�s aux sommets, on va devoir associer chaque sommet � un UV
            // Au final, on trouvera un grand nombre de sommets dupliqu�s et on se passera d'index buffer,
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
            // Lorsque les UV son appair�s aux sommets, on a une correspondance 1 � 1, ce qui est optimal
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
     * S�rialization
     */
    std::unique_ptr<char> Mesh::Serialize(uint32_t& output_size)
    {
        // Taille du buffer de mat�riaux
        uint32_t material_buffer_size = 0;
        for(auto& material : this->materials)
            material_buffer_size += static_cast<uint32_t>(sizeof(uint8_t) + material.first.size() + sizeof(uint32_t) + material.second.size() * sizeof(uint32_t));

        // Calcul de la taille du buffer de sortie
        SERIALIZED_HEADER header;
        header.name_length = static_cast<uint8_t>(this->name.size());
        header.skeleton_length = static_cast<uint8_t>(this->skeleton.size());
        header.material_buffer_size = material_buffer_size;
        header.vertex_count = static_cast<uint32_t>(this->vertex_buffer.size());
        header.index_count = static_cast<uint32_t>(this->index_buffer.size());
        header.uv_count = static_cast<uint32_t>(this->uv_buffer.size());
        header.uv_index_count = static_cast<uint32_t>(this->uv_index.size());
        header.deformer_count = static_cast<uint32_t>(this->deformers.size());
        header.normals = !this->normal_buffer.empty();

        uint32_t vertex_buffer_size = static_cast<uint32_t>(this->GetVertexBufferSize());
        uint32_t index_buffer_size = static_cast<uint32_t>(this->GetIndexBufferSize());
        uint32_t uv_buffer_size = static_cast<uint32_t>(this->GetUvBufferSize());
        uint32_t uv_index_buffer_size = static_cast<uint32_t>(this->GetUvIndexBufferSize());

        // La taille du buffer de d�formeurs est variable
        uint32_t deformer_buffer_size = 0;
        for(auto& deformer : this->deformers)
            deformer_buffer_size += sizeof(uint8_t) + deformer.Size() * (sizeof(uint32_t) + sizeof(float));

        // Allocation du buffer de s�rialization
        output_size = sizeof(header) + header.name_length + header.skeleton_length + vertex_buffer_size + index_buffer_size
                    + uv_buffer_size + uv_index_buffer_size + material_buffer_size + deformer_buffer_size;
        if(header.normals) output_size += vertex_buffer_size;
        std::unique_ptr<char> output(new char[output_size]);

        // Insertion du header
        std::memcpy(output.get(), &header, sizeof(SERIALIZED_HEADER));

        // Insertion du nom
        size_t offset = sizeof(SERIALIZED_HEADER);
        std::memcpy(output.get() + offset, this->name.data(), header.name_length);
        offset += header.name_length;

        if(!this->skeleton.empty()) {
            // Insertion du Nom du squelette rattach� au mod�le
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

        if(!this->materials.empty()) {
            // Insertion des mat�riaux
            for(auto& material : this->materials) {

                // Nom du mat�riau
                uint8_t material_name_length = static_cast<uint8_t>(material.first.size());
                std::memcpy(output.get() + offset, &material_name_length, sizeof(uint8_t));
                offset += sizeof(uint8_t);
                std::memcpy(output.get() + offset, material.first.data(), material_name_length);
                offset += material_name_length;

                // Faces auxquelles s'appliquent le mat�riau
                uint32_t face_index_buffer_size = static_cast<uint32_t>(material.second.size() * sizeof(uint32_t));
                std::memcpy(output.get() + offset, &face_index_buffer_size, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                std::memcpy(output.get() + offset, material.second.data(), face_index_buffer_size);
                offset += face_index_buffer_size;
            }
        }

        if(!this->deformers.empty()) {
            // Insertion des d�formeurs
            for(auto& deformer : this->deformers) {
                std::vector<char> serialized = deformer.Serialize();
                std::memcpy(output.get() + offset, serialized.data(), serialized.size());
                offset += serialized.size();
            }
        }

        // Renvoi du r�sultat
        return output;
    }

    /**
     * D�s�rialization
     */
    size_t Mesh::Deserialize(const char* data)
    {
        // Nettoyage des sub meshes
        this->sub_meshes.clear();

        // Lecture du header
        uint32_t offset = 0;
        SERIALIZED_HEADER header;
        header = *reinterpret_cast<const SERIALIZED_HEADER*>(data + offset);
        offset += sizeof(SERIALIZED_HEADER);

        // Lecture du nom
        this->name = std::string(data + offset, data + offset + header.name_length);
        offset += header.name_length;

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

        if(header.material_buffer_size > 0) {
            // Lecture des mat�riaux
            uint32_t bytes_read = 0;
            while(bytes_read < header.material_buffer_size) {

                // Nom du mat�riau
                uint8_t material_name_length = data[offset];
                offset += sizeof(uint8_t);
                std::string material_name = std::string(data + offset, data + offset + material_name_length);
                offset += material_name_length;

                // Faces auxquelles s'appliquent le mat�riau
                uint32_t face_index_buffer_size = *reinterpret_cast<const uint32_t*>(data + offset);
                offset += sizeof(uint32_t);
                std::vector<uint32_t> face_index_buffer(face_index_buffer_size / sizeof(uint32_t));
                std::memcpy(face_index_buffer.data(), &data[offset], face_index_buffer_size);
                offset += face_index_buffer_size;

                // On ajoute le mat�riau lu
                this->materials.push_back(std::pair<std::string, std::vector<uint32_t>>(material_name, face_index_buffer));
                bytes_read += sizeof(uint8_t) + material_name_length + sizeof(uint32_t) + face_index_buffer_size;
            }
        }

        if(header.deformer_count > 0) {
            // Lecture des d�formeurs
            this->deformers.resize(header.deformer_count);
            for(uint32_t i=0; i<header.deformer_count; i++)
                offset += this->deformers[i].Deserialize(data + offset);
        }

        // On renvoie le nombre d'octets lus
        return offset;
    }

    /**
     * S�rialization d'un mat�riau
     */
    std::unique_ptr<char> Mesh::MATERIAL::Serialize(uint32_t& output_size)
    {
        output_size = static_cast<uint32_t>(sizeof(Maths::Vector4) * 3 + sizeof(float) + sizeof(uint8_t) + this->texture.size());
        std::unique_ptr<char> output(new char[output_size]);
        uint32_t offset = 0;
        *reinterpret_cast<Maths::Vector4*>(output.get() + offset) = this->ambient;
        offset += sizeof(Maths::Vector4);
        *reinterpret_cast<Maths::Vector4*>(output.get() + offset) = this->diffuse;
        offset += sizeof(Maths::Vector4);
        *reinterpret_cast<Maths::Vector4*>(output.get() + offset) = this->specular;
        offset += sizeof(Maths::Vector4);
        *reinterpret_cast<float*>(output.get() + offset) = this->transparency;
        offset += sizeof(float);
        output.get()[offset] = static_cast<char>(this->texture.size());
        offset++;
        std::memcpy(output.get() + offset, this->texture.data(), this->texture.size());

        return output;
    }

    /**
     * D�s�rialization d'un mat�riau
     */
    size_t Mesh::MATERIAL::Deserialize(const char* data)
    {
        uint32_t offset = 0;
        this->ambient = *reinterpret_cast<const Maths::Vector4*>(data + offset);
        offset += sizeof(Maths::Vector4);
        this->diffuse = *reinterpret_cast<const Maths::Vector4*>(data + offset);
        offset += sizeof(Maths::Vector4);
        this->specular = *reinterpret_cast<const Maths::Vector4*>(data + offset);
        offset += sizeof(Maths::Vector4);
        this->transparency = *reinterpret_cast<const float*>(data + offset);
        offset += sizeof(float);
        uint8_t name_length = *(data + offset);
        offset++;
        if(name_length) this->texture = std::string(data + offset, data + offset + name_length);
        return offset + name_length;
    }
}