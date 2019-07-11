#include "ModelLoader.h"

namespace Engine
{
    bool ModelLoader::LoadFromFile(std::string file_name)
    {
        #if defined(_DEBUG)
        std::cout << "Loading file : " << file_name << std::endl;
        #endif

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(file_name.c_str(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

        if(!scene) return false;

        std::string file_directory = this->GetDirectory(file_name);
        if(!this->LoadMaterials(scene, file_directory)) return false;

        bool loaded = this->ProcessNode(scene->mRootNode, scene);
        this->textures.clear();

        return loaded;
    }

    std::string ModelLoader::GetDirectory(std::string& file_name)
    {
        size_t pos = file_name.find_last_of('/');
        if(pos == std::string::npos) return std::string();
        return file_name.substr(0, pos);
    }

    bool ModelLoader::LoadMaterials(const aiScene* scene, std::string directory)
    {
        for (unsigned int i=0; i<scene->mNumMaterials; ++i)
	    {
            #if defined(_DEBUG)
            std::cout << "Display material[" << i << "] informations :" << std::endl;
            this->ShowMaterialInformation(scene->mMaterials[i]);
            #endif

            // On cherche à récupérer la texture diffuse du matériau
		    aiString path;
		    aiReturn texFound = scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            if(texFound == AI_SUCCESS) {
                std::string spath = path.data;
                std::string texture_file = directory + '/' + spath;
                Tools::IMAGE_MAP image = Engine::Tools::LoadImageFile(texture_file);

                this->textures[i] = Vulkan::GetInstance()->CreateTexture(image);
                if(this->textures[i] == UINT32_MAX) return false;
            }
        }

        return true;
    }

    #if defined(_DEBUG)
    void ModelLoader::ShowMaterialInformation(const aiMaterial* pMaterial)
    {
        aiString name;
        if(AI_SUCCESS == pMaterial->Get(AI_MATKEY_NAME, name))
        {
            std::cout << "   Name: " << name.data << std::endl;
        }
        aiColor3D color;
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color) )
        {
            std::cout << "   Ambient color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) )
        {
            std::cout << "   Diffuse color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) )
        {
            std::cout << "   Emissive color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_REFLECTIVE, color) )
        {
            std::cout << "   Reflective color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) )
        {
            std::cout << "   Specular color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
        }
        float value;
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_SHININESS, value) )
        {
            std::cout << "   Shininess: " << value << std::endl;
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_SHININESS_STRENGTH, value) )
        {
            std::cout << "   Shininess strength: " << value << std::endl;
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, color) )
        {
            std::cout << "   Transparent color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
        }
        int intValue;
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_ENABLE_WIREFRAME, intValue) )
        {
            if( intValue == 0 )
            {
                std::cout << "   Wireframe: Disabled" << std::endl;
            }
            else if( intValue == 1 )
            {
                std::cout << "   Wireframe: Enabled" << std::endl;
            }
            else
            {
                std::cout << "   Wireframe: Unexpected value" << std::endl;
            }
        }
        if( AI_SUCCESS == pMaterial->Get(AI_MATKEY_SHADING_MODEL, intValue) )
        {
            std::cout << "   Shading model: " << intValue << std::endl;
        }
        unsigned int aux = pMaterial->GetTextureCount(aiTextureType_AMBIENT);
        if( aux > 0 )
        {
            std::cout << "   Number of ambient textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_AMBIENT, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_DIFFUSE);
        if( aux > 0 )
        {
            std::cout << "   Number of diffuse textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_DIFFUSE, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_DISPLACEMENT);
        if( aux > 0 )
        {
            std::cout << "   Number of displacement textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_DISPLACEMENT, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_EMISSIVE);
        if( aux > 0 )
        {
            std::cout << "   Number of emissive textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_EMISSIVE, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_HEIGHT);
        if( aux > 0 )
        {
            std::cout << "   Number of height textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_HEIGHT, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_LIGHTMAP);
        if( aux > 0 )
        {
            std::cout << "   Number of lightmap textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_LIGHTMAP, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_NORMALS);
        if( aux > 0 )
        {
            std::cout << "   Number of normals textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_NORMALS, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_OPACITY);
        if( aux > 0 )
        {
            std::cout << "   Number of opacity textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_OPACITY, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_REFLECTION);
        if( aux > 0 )
        {
            std::cout << "   Number of reflection textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_REFLECTION, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_SHININESS);
        if( aux > 0 )
        {
            std::cout << "   Number of shininess textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_SHININESS, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_SPECULAR);
        if( aux > 0 )
        {
            std::cout << "   Number of specular textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_SPECULAR, i);
            }
        }
        aux = pMaterial->GetTextureCount(aiTextureType_UNKNOWN);
        if( aux > 0 )
        {
            std::cout << "   Number of unknown textures: " << aux << std::endl;
            for( unsigned int i = 0; i < aux; i++ )
            {
                this->ShowTextureInformation(pMaterial, aiTextureType_UNKNOWN, i);
            }
        }
    }

    void ModelLoader::ShowTextureInformation(const aiMaterial* pMaterial, aiTextureType pType, unsigned int pTextureNumber)
    {
        aiString path;
        aiTextureMapping mapping;
        unsigned int uvindex;
        float blend;
        aiTextureOp op;
        aiTextureMapMode mapmode;
        std::cout << "      Information of texture " << pTextureNumber << std::endl;
        if( AI_SUCCESS == pMaterial->GetTexture( pType, pTextureNumber, &path, &mapping, &uvindex, &blend, &op, &mapmode ) )
        {
            std::cout << "         Path: " << path.data << std::endl;
            std::cout << "         UV index: " << uvindex << std::endl;
            std::cout << "         Blend: " << blend << std::endl;
        }
        else
        {
            std::cout << "         Impossible to get the texture" << std::endl;
        }
    }
    #endif

    bool ModelLoader::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for(uint32_t i=0; i<node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            if(!this->ProcessMesh(mesh, scene)) return false;
        }

        for(uint32_t i=0; i<node->mNumChildren; i++) {
            if(!this->ProcessNode(node->mChildren[i], scene)) return false;
        }

        return true;
    }

    bool ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vulkan::VERTEX> vertex_data;
		for (uint32_t i=0; i<mesh->mNumVertices; i++) {

            Vulkan::VERTEX vertex;
            vertex.vertex[0] = mesh->mVertices[i].x;
            vertex.vertex[1] = mesh->mVertices[i].y;
            vertex.vertex[2] = mesh->mVertices[i].z;
            vertex.vertex[3] = 1.0f;

            if(mesh->HasTextureCoords(0)) {
                vertex.texture_coordinates[0] = mesh->mTextureCoords[0][i].x;
                vertex.texture_coordinates[1] = mesh->mTextureCoords[0][i].y;
            }

            /*if(scene->mMeshes[m]->HasVertexColors(0)) {
                vertex.color[0] = scene->mMeshes[m]->mColors[0][v].r;
                vertex.color[0] = scene->mMeshes[m]->mColors[0][v].g;
                vertex.color[0] = scene->mMeshes[m]->mColors[0][v].b;
            }*/

            vertex_data.push_back(vertex);
        }
        
        uint32_t vertex_buffer_id = Vulkan::GetInstance()->CreateVertexBuffer(vertex_data);

        std::vector<uint32_t> index_buffer;
		for (uint32_t f=0; f<mesh->mNumFaces; f++)
			for (uint32_t i=0; i<mesh->mFaces[f].mNumIndices; i++)
                index_buffer.push_back(mesh->mFaces[f].mIndices[i]);

        uint32_t index_buffer_id = Vulkan::GetInstance()->CreateIndexBuffer(index_buffer);

        if(textures.count(mesh->mMaterialIndex) > 0) {
            uint32_t texture_id = textures[mesh->mMaterialIndex];
            uint32_t model_id = Vulkan::GetInstance()->CreateModel(vertex_buffer_id, texture_id, index_buffer_id);

            #if defined(_DEBUG)
            std::cout << "Mesh loaded : " << mesh->mName.data << std::endl;
            #endif

            return model_id != UINT32_MAX;
        }else{
            // Si un mesh n'a pas de texture diffuse, on ne le charge pas mais on sort quand-même en succès
            return true;
        }
    }
}