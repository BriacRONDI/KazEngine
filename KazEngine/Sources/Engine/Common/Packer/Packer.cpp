#include "Packer.h"

namespace Engine
{
    /**
     * Affectation par déplacement
     */
    Packer::DATA& Packer::DATA::operator=(DATA&& other)
    {
        if(&other != this) {
            this->type = other.type;
            this->size = other.size;
            this->position = other.position;
            this->name = std::move(other.name);
            this->children = std::move(other.children);
            other.type = DATA_TYPE::UNDEFINED;
            other.size = 0;
            other.position = 0;
        }
        return *this;
    }

    /**
     * Renvoie la taille de l'entête du noeud
     */
    uint32_t Packer::DATA::HeaderSize() const
    {
        return static_cast<uint32_t>(sizeof(DATA_TYPE) + sizeof(uint8_t) + this->name.size() + sizeof(uint32_t));
    }

    /**
    * Sérialization d'un paquet
    */
    std::vector<char> Packer::DATA::Serialize() const
    {
        // Donnée résultante
        std::vector<char> output;
        
        // Insertion du type
        output.insert(output.end(), &this->type, &this->type + sizeof(DATA_TYPE));

        // Insertion du nom
        uint8_t name_length = static_cast<uint8_t>(this->name.size());
        output.insert(output.end(), &name_length, &name_length + sizeof(uint8_t));
        output.insert(output.end(), this->name.begin(), this->name.end());

        // Insertion de la taille de la donnée
        std::vector<char> serialized_size = Packer::SerializeUint32(this->size);
        output.insert(output.end(), serialized_size.begin(), serialized_size.end());

        // Pour chaque noeud imbriqué
        for(auto& child : this->children) {

            // On sérialize le noeud imbriqué et on l'insère
            std::vector<char> serialized_child = child.Serialize();
            output.insert(output.end(), serialized_child.begin(), serialized_child.end());
        }
        
        // On renvoie le résultat
        return output;
    }

    /*
     * Lecture du package depuis la mémoire
     * Si data est fourni, il s'agit d'un sous-segment de mémoire qui est lui-même un package
     */
    std::vector<Packer::DATA> Packer::UnpackMemory(std::vector<char> const& memory, uint32_t position, uint32_t size)
    {
        std::vector<DATA> package;

        uint32_t final_position;
        if(!position && !size && memory.size() > 0) final_position = static_cast<uint32_t>(memory.size());
        else final_position = position + size;
        
        while(position < final_position) {

            // On conserve la position du noeud
            uint32_t node_position = position;
            
            // Le premier octet est le type de donnée
            DATA_TYPE type = static_cast<DATA_TYPE>(memory[position]);
            position++;

            // Ensuite vient directement la taille du nom de la donnée
            uint8_t name_length = memory[position];
            position++;

            // Et donc le nom de la donée
            std::string name;
            if(name_length > 0) name = std::string(memory.begin() + position, memory.begin() + position + name_length);
            position += name_length;

            // Les 4 octets suivants représentent la taille de la donnée
            uint32_t data_size = *reinterpret_cast<const uint32_t*>(&memory[position]);
            if(Tools::IsBigEndian()) data_size = _byteswap_ulong(data_size);
            position += sizeof(uint32_t);

            // Si la donnée est un noeud parent, on lit son contenu récursivement
            std::vector<DATA> children;
            if(type == DATA_TYPE::PARENT_NODE) children = Packer::UnpackMemory(memory, position, data_size);

            // On ajoute la donnée au résultat
            package.push_back({type, data_size, node_position, name, children});

            // On passe à l'élément suivant
            position += data_size;
        }

        // Le package est prêt
        return package;
    }

    /**
     * Ajout d'une donnée dans un package en mémoire
     */
    bool Packer::PackToMemory(std::vector<char>& memory, std::string const& path, DATA_TYPE const type,
                              std::string const& name, std::unique_ptr<char> const& data, uint32_t data_size)
    {
        // Lecture de la table des données
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);

        // Recherche du chemin
        DATA pack = Packer::FindPackedItem(package, path);

        // Seuls le noeud racine ou un conteneur peuvent accueillir la donnée
        if(pack.type != DATA_TYPE::PARENT_NODE && pack.type != DATA_TYPE::ROOT_NODE) return false;

        // On créé le noeud à insérer
        DATA node;
        node.name = name;
        node.type = type;
        node.size = data_size;
        if(pack.type == DATA_TYPE::ROOT_NODE) node.position = pack.size;
        else node.position = pack.position + pack.HeaderSize() + pack.size;

        // Insertion du noeud
        std::vector<char> serialized_node = node.Serialize();
        memory.insert(memory.begin() + node.position, serialized_node.begin(), serialized_node.end());

        // Insertion de la donnée, s'il y en a une
        if(node.size) memory.insert(memory.begin() + node.position + serialized_node.size(), data.get(), data.get() + data_size);

        // On change la taille de tous les conteneurs parents
        Packer::UpdatePackSize(memory, package, path, static_cast<uint32_t>(serialized_node.size() + data_size));

        return true;
    }

    void Packer::UpdatePackSize(std::vector<char>& memory, std::vector<DATA> const& package, std::string path, int32_t size_change)
    {
        // On est à la racine
        if(!path.size() || path == "/") return;

        // Nom du premier élément du chemin
        size_t pos = path.find_first_of('/');
        if(pos == 0) {
            path = path.substr(1);
            pos = path.find_first_of('/');
        }

        std::string name;
        if(pos == std::string::npos) name = path;
        else name = path.substr(0, pos);

        // On cherche si cet élément est dans l'arborescence
        for(DATA const& pack : package) {
            if(pack.name == name) {

                // Si l'élément qu'on cherche est le dernier du chemin, on le met à jour et on s'arrête
                if(pos == std::string::npos) {
                    uint32_t size_position = static_cast<uint32_t>(pack.position + sizeof(DATA_TYPE) + sizeof(uint8_t) + pack.name.size());
                    std::vector<char> new_size = Packer::SerializeUint32(pack.size + size_change);
                    std::copy(new_size.begin(), new_size.end(), memory.begin() + size_position);
                    return;
                }

                // Sinon, on met à jour les sous éléments
                if(pack.children.size() > 0) Packer::UpdatePackSize(memory, pack.children, path.substr(pos + 1), size_change);
            }
        }
    }

    Packer::DATA Packer::FindPackedItem(std::vector<DATA> const& package, std::string path)
    {
        // On est à la racine
        if(!path.size() || path == "/") {
            uint32_t root_size = 0;
            for(auto& pack : package) root_size += pack.HeaderSize() + pack.size;
            return ROOT_PACKER_NODE(root_size);
        }

        // Nom du premier élément du chemin
        size_t pos = path.find_first_of('/');
        if(pos == 0) {
            path = path.substr(1);
            pos = path.find_first_of('/');
        }

        std::string name;
        if(pos == std::string::npos) name = path;
        else name = path.substr(0, pos);

        // On cherche si cet élément est dans l'arborescence
        for(DATA const& pack : package) {
            if(pack.name == name) {

                // Si l'élément qu'on cherche est le dernier du chemin, on le renvoie
                if(pos == std::string::npos) return pack;

                // Sinon, on explore les sous éléments, s'il y en a
                if(pack.children.size() > 0) {
                    DATA child = Packer::FindPackedItem(pack.children, path.substr(pos + 1));
                    if(child.size > 0) return child;
                }
            }
        }

        // Aucun élément trouvé
        return {};
    }

    /**
     * Lecture d'un string
     */
    std::string Packer::ParseString(std::vector<char> const& memory, uint32_t position, uint32_t size)
    {
        // On renvoie une string vide si la taille de la donnée est nulle
        if(!size) return std::string();

        // Construction de la string à partir des positions de début et de fin de la donnée
        return std::string(memory.begin() + position, memory.begin() + position + size);
    }

    /**
     * Lecture d'un tableau de Vector3
     */
    std::vector<Vector3> Packer::ParseVector3Array(const char* data, size_t size)
    {
        std::vector<Vector3> output;

        // Allocation du buffer
        uint32_t vector_count = static_cast<uint32_t>(size / sizeof(Vector3));
        if(vector_count * sizeof(Vector3) != size) return output;
        output.resize(vector_count);

        // Lecture du buffer
        if(Tools::IsBigEndian())
            for(uint32_t i=0; i<vector_count; i++)
                output[i].Deserialize(data + i * sizeof(Vector3));

        else std::memcpy(output.data(), data, size);

        return output;
    }

    /**
     * Lecture d'un tableau de Vector2
     */
    std::vector<Vector2> Packer::ParseVector2Array(const char* data, size_t size)
    {
        std::vector<Vector2> output;

        // Allocation du buffer
        uint32_t vector_count = static_cast<uint32_t>(size / sizeof(Vector2));
        if(vector_count * sizeof(Vector2) != size) return output;
        output.resize(vector_count);

        // Lecture du buffer
        if(Tools::IsBigEndian())
            for(uint32_t i=0; i<vector_count; i++)
                output[i].Deserialize(data + i * sizeof(Vector2));

        else std::memcpy(output.data(), data, size);

        return output;
    }

    /**
     * Lecture d'un tableau de uint32_t
     */
    std::vector<uint32_t> Packer::ParseUint32Array(const char* data, size_t size)
    {
        std::vector<uint32_t> output;

        // Allocation du buffer
        uint32_t count = static_cast<uint32_t>(size / sizeof(uint32_t));
        if(count * sizeof(uint32_t) != size) return output;
        output.resize(count);

        // Lecture du buffer
        if(Tools::IsBigEndian()) {
            for(uint32_t i=0; i<count; i++) {
                uint32_t value = *reinterpret_cast<const uint32_t*>(data + i * sizeof(uint32_t));
                output[i] = Tools::BytesSwap(value);
            }
        }

        else std::memcpy(output.data(), data, size);

        return output;
    }

    /**
     * Sérialization d'un tableau de Vector3
     */
    std::vector<char> Packer::SerializeVector3Array(std::vector<Vector3> const& array)
    {
        size_t buffer_size = array.size() * sizeof(Vector3);
        std::vector<char> output(buffer_size);
        if(Tools::IsBigEndian()) {
            for(uint32_t i=0; i<array.size(); i++) {
                std::unique_ptr<char> serialized = array[i].Serialize();
                std::memcpy(output.data() + i * sizeof(Vector3), serialized.get(), sizeof(Vector3));
            }
        }else{
            std::memcpy(output.data(), array.data(), buffer_size);
        }
        return output;
    }

    /**
     * Sérialization d'un tableau de Vector2
     */
    std::vector<char> Packer::SerializeVector2Array(std::vector<Vector2> const& array)
    {
        size_t buffer_size = array.size() * sizeof(Vector2);
        std::vector<char> output(buffer_size);
        if(Tools::IsBigEndian()) {
            for(uint32_t i=0; i<array.size(); i++) {
                std::unique_ptr<char> serialized = array[i].Serialize();
                std::memcpy(output.data() + i * sizeof(Vector2), serialized.get(), sizeof(Vector2));
            }
        }else{
            std::memcpy(output.data(), array.data(), buffer_size);
        }
        return output;
    }

    /**
     * Sérialization d'un tableau de uint32_t
     */
    std::vector<char> Packer::SerializeUint32Array(std::vector<uint32_t> const& array)
    {
        size_t buffer_size = array.size() * sizeof(uint32_t);
        std::vector<char> output(buffer_size);
        if(Tools::IsBigEndian()) {
            for(uint32_t i=0; i<array.size(); i++) {
                uint32_t value = Tools::BytesSwap(array[i]);
                std::memcpy(output.data() + i * sizeof(uint32_t), &value, sizeof(uint32_t));
            }
        }else{
            std::memcpy(output.data(), array.data(), buffer_size);
        }
        return output;
    }

    /**
     * Sérialisation d'un uint32_t
     */
    std::vector<char> Packer::SerializeUint32(uint32_t value)
    {
        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            value =_byteswap_ulong(value);
            #endif
        }
        
        char* tmp = reinterpret_cast<char*>(&value);
        return std::vector<char>(tmp, tmp + sizeof(uint32_t));
    }

    /**
     * Lecture d'un uint32_t
     */
    uint32_t Packer::ParseUint32(std::vector<char> const& memory, const size_t position)
    {
        uint32_t value = *reinterpret_cast<const uint32_t*>(&memory[position]);

        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            return _byteswap_ulong(value);
            #endif
        }else{
            return value;
        }
    }

    /**
     * Lecture d'un int32_t
     */
    int32_t Packer::ParseInt32(std::vector<char> const& memory, const size_t position)
    {
        int32_t value = *reinterpret_cast<const int32_t*>(&memory[position]);

        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            return _byteswap_ulong(value);
            #endif
        }else{
            return value;
        }
    }

    /**
     * Lecture d'un uint64_t
     */
    uint64_t Packer::ParseUint64(std::vector<char> const& memory, const size_t position)
    {
        uint64_t value = *reinterpret_cast<const uint64_t*>(&memory[position]);

        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            return _byteswap_uint64(value);
            #endif
        }else{
            return value;
        }
    }

    /**
     * Lecture d'un int64_t
     */
    int64_t Packer::ParseInt64(std::vector<char> const& memory, const size_t position)
    {
        int64_t value = *reinterpret_cast<const int64_t*>(&memory[position]);

        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            return _byteswap_uint64(value);
            #endif
        }else{
            return value;
        }
    }

    /**
     * Lecture d'un int16_t
     */
    int16_t Packer::ParseInt16(std::vector<char> const& memory, const size_t position)
    {
        int16_t value = *reinterpret_cast<const int16_t*>(&memory[position]);

        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            return _byteswap_ushort(value);
            #endif
        }else{
            return value;
        }
    }

    /**
     * Lecture d'un float64_t
     */
    double Packer::ParseFloat64(std::vector<char> const& memory, const size_t position)
    {
        if(Tools::IsBigEndian()) {
            #if defined(_WIN32)
            uint64_t value = *reinterpret_cast<const uint64_t*>(&memory[position]);
            value =_byteswap_uint64(value);
            return *reinterpret_cast<const double*>(&value);
            #endif
        }else{
            return *reinterpret_cast<const double*>(&memory[position]);
        }
    }
}