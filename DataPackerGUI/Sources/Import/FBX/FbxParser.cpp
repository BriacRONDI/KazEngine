#include "FbxParser.h"

namespace DataPackerGUI
{
    std::vector<char> FbxParser::ParseData(std::vector<char> const& data, std::string const& texture_directory, std::string const& package_directory)
    {
        this->data = &data;

        // On vérifie l'emprunte FBX
        std::cout << "Checking FBX tag : ";

        if(!this->isBinaryFbx()) {
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::RED);
            std::cout << "FAILED" << std::endl;
            Log::Terminal::SetTextColor();
            return {};
        }

        Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
        std::cout << "OK" << std::endl;
        Log::Terminal::SetTextColor();

        std::cout << "FBX Version : ";
        Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::BLUE);
        this->version = this->GetVersion();
        std::cout << this->version << std::endl;
        Log::Terminal::SetTextColor();

        std::cout << "Lecture du fichier : ";
        std::vector<FBX_NODE> nodes;
        size_t position = 27;   // Les données utiles commencent à la position 27
        while(!this->EndOfContent(position)) {
            FBX_NODE node = this->ParseNode(position);
            if(!this->IsNullNode(node)) nodes.push_back(node);
        }

        Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
        std::cout << "OK" << std::endl;
        Log::Terminal::SetTextColor();

        this->next_referenced_bone_id = 0;

        this->ParseConnections(nodes);
        this->ParseTextures(nodes);
        this->ParseMaterials(nodes);
        this->ParseLimb(nodes);
        this->ParseSkeletons(nodes);
        this->ParseMeshGeometry(nodes);
        this->ParseAnimationCurveNode(nodes);
        this->ParseAnimationCurve(nodes);
        this->ParseAnimationLayer(nodes);
        this->ParseAnimationStack(nodes);
        this->ParseMeshes(nodes);

        bool skeep = false;
        for(auto& tree : this->bone_trees)
            this->RebuildBoneTree(tree.second, skeep);
        this->ComputeAnimations();

        // Création des conteneurs pour textures et meshes
        std::vector<char> memory;
        std::vector<std::string> added_textures;
        std::vector<std::string> added_materials;
        // DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::PARENT_NODE, "textures", {}, 0);
        // DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::PARENT_NODE, "materials", {}, 0);
        // DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::PARENT_NODE, "meshes", {}, 0);

        // if(!this->bone_trees.empty())
        //    DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::PARENT_NODE, "bones", {}, 0);

        // Empaquetage des squelettes
        for(auto const& tree : this->bone_trees) {
            std::vector<char> serialized = tree.second.Serialize();
            std::unique_ptr<char> serialized_tree(serialized.data());
            std::cout << "Empaquetage du squelette [" << tree.first << "] : ";
            DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::BONE_TREE, tree.first, serialized_tree, static_cast<uint32_t>(serialized.size()));
            serialized_tree.release();

            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();
        }

        for(auto& mesh : this->meshes) {

            // Add skeleton dependancy
            std::vector<std::string> dependancies;
            if(!mesh.skeleton.empty()) dependancies.push_back(package_directory + "/" + mesh.skeleton);

            // Si le matériau n'est pas déjà empaqueté, on s'en charge
            for(auto& material : mesh.materials) {

                // Add material dependancy
                dependancies.push_back(package_directory + "/" + material.first);

                if(std::find(added_materials.begin(), added_materials.end(), material.first) == added_materials.end()
                && this->engine_materials.count(material.first) > 0) {

                    // Empaquetage du matériau
                    uint32_t serialized_material_size;
                    std::unique_ptr<char> serialized_material = this->engine_materials[material.first].Serialize(serialized_material_size);
                    std::cout << "Empaquetage de matériau [" << material.first << "] : ";
                    std::vector<std::string> textures;
                    if(!this->engine_materials[material.first].texture.empty()) textures.push_back(package_directory + "/" + this->engine_materials[material.first].texture);
                    DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::MATERIAL_DATA, material.first, serialized_material, serialized_material_size, textures);

                    Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
                    std::cout << "OK" << std::endl;
                    Log::Terminal::SetTextColor();

                    // On valide la présence du matériau dans le package
                    added_materials.push_back(material.first);

                    // Si le matériau est associé à une texture, on la stocke également
                    if(!this->engine_materials[material.first].texture.empty()
                    && std::find(added_textures.begin(), added_textures.end(), this->engine_materials[material.first].texture) == added_textures.end()) {
                        std::string filename = this->engine_materials[material.first].texture;

                        // Recherche et lecture du fichier
                        std::cout << "Empaquetage de texture [" << filename << "] : ";
                        std::vector<char> file_content;
                        for(auto& texture : this->textures) {
                            if(Tools::GetFileName(texture.RelativeFilename) == filename) {
                                file_content = Tools::GetBinaryFileContents(texture_directory + '/' + texture.RelativeFilename);
                                break;
                            }
                        }
                        if(file_content.empty()) {
                            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::RED);
                            std::cout << "Fichier introuvable" << std::endl;
                            Log::Terminal::SetTextColor();

                        }else{
                            // Empaquetage de la texture
                            std::unique_ptr<char> file_content_ptr(file_content.data());
                            DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::IMAGE_FILE, filename,
                                                             file_content_ptr, static_cast<uint32_t>(file_content.size()));
                            file_content_ptr.release();
                        
                            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
                            std::cout << "OK" << std::endl;
                            Log::Terminal::SetTextColor();

                            // On valide la présence de la texture dans le package
                            added_textures.push_back(filename);
                        }
                    }
                }
            }

            // Empaquetage du mesh
            uint32_t serialized_mesh_size;
            std::unique_ptr<char> serialized_mesh = mesh.Serialize(serialized_mesh_size);
            std::cout << "Empaquetage de mesh [" << mesh.name << "] : ";
            DataPacker::Packer::PackToMemory(memory, "/", DataPacker::Packer::DATA_TYPE::MESH_DATA, mesh.name, serialized_mesh, serialized_mesh_size, dependancies);

            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();
        }

        std::cout << "Fin du traitement : ";
        Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
        std::cout << "Succès" << std::endl;
        Log::Terminal::SetTextColor();

        return memory;
    }

    FbxParser::FBX_NODE FbxParser::GetRootModel(FBX_NODE const& node, std::vector<FBX_NODE> const& models)
    {
        for(auto parent : this->connections[node.id].parents)
            for(FBX_NODE const& model : models)
                if(model.id == parent.id)
                    return this->GetRootModel(model, models);

        return node;
    }

    void FbxParser::ParseLimb(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const models = this->GetObjectNode("Model", nodes);

        for(auto& limb : models) {
            if(limb.attribute_type != "LimbNode" && limb.attribute_type != "Null") continue;

            FBX_NODE root_node = this->GetRootModel(limb, models);
            Model::Bone root_bone;

            this->bone_trees[root_node.attribute_name] = this->GetBoneTree(limb, root_bone, models);
            return;
        }
    }

    Model::Bone FbxParser::GetBoneTree(FBX_NODE const& node, Model::Bone const& parent, std::vector<FBX_NODE> const& models)
    {
        Model::Bone bone = this->CreateBone(node, parent);

        for(auto child : this->connections[node.id].children) {
            for(FBX_NODE const& model : models) {
                if(model.id == child.id) {
                    if(model.attribute_type == "LimbNode") {
                        Model::Bone child_bone = this->GetBoneTree(model, bone, models);
                        bone.children.push_back(child_bone);
                    }
                }
            }
        }

        return bone;
    }

    Model::Bone FbxParser::CreateBone(FBX_NODE const& node, Model::Bone const& parent)
    {
        Model::Bone bone;

        if(this->referenced_bones.count(node.id) == 0) {
            this->referenced_bones[node.id] = this->next_referenced_bone_id;
            bone.index = this->next_referenced_bone_id;
            this->next_referenced_bone_id++;
        }else{
            bone.index = this->referenced_bones[node.id];
        }

        bone.name = node.attribute_name;
        Maths::Matrix4x4 parent_transformation = IDENTITY_MATRIX;
        bone.transformation = this->ComputeTransformation(node, parent_transformation);
        return bone;
    }

    Model::Bone& FbxParser::FindBone(uint32_t bone_id, Model::Bone& parent)
    {
        if(parent.index == bone_id) return parent;
        for(auto& child : parent.children) {
            Model::Bone& bone = this->FindBone(bone_id, child);
            if(bone.index != UINT32_MAX) return bone;
        }

        return this->null_bone;
    }

    Model::Bone FbxParser::RebuildBoneTree(Model::Bone& bone, bool& skeep_me)
    {
        std::vector<Model::Bone> new_children;
        skeep_me = true;

        for(auto child : bone.children) {
            bool skeep_child = false;
            Model::Bone child_bone = this->RebuildBoneTree(child, skeep_child);
            if(!skeep_child) new_children.push_back(child_bone);
        }
        
        if(new_children.size() > 0) {
            bone.children = new_children;
            skeep_me = false;
        }else{
            bone.children.clear();
        }

        if(this->used_bones.count(bone.index)) {
            bone.index = this->used_bones[bone.index];
            skeep_me = false;
        }else{
            this->unused_bones[bone.index] = this->next_used_bone_id;
            bone.index = this->next_used_bone_id;
            this->next_used_bone_id++;
        }
        
        return bone;
    }

    void FbxParser::ComputeAnimations()
    {
        for(auto& layer : this->animation_layers) {
            for(auto& node : layer.animation_nodes) {

                if(node.second.curve_nodes.size() == 0) continue;
            
                std::array<std::vector<Model::KEYFRAME>, 3> translations, rotations, scalings;
            
                if(node.second.curve_nodes.count("T")) translations = this->ParseCurveNode(node.second.curve_nodes["T"]);
                if(node.second.curve_nodes.count("R")) rotations = this->ParseCurveNode(node.second.curve_nodes["R"]);
                if(node.second.curve_nodes.count("S")) scalings = this->ParseCurveNode(node.second.curve_nodes["S"]);

                uint32_t ref_id = UINT32_MAX;
                uint32_t bone_id = UINT32_MAX;
                if(this->referenced_bones.count(node.second.model.id)) ref_id = this->referenced_bones[node.second.model.id];
                if(this->used_bones.count(ref_id)) bone_id = this->used_bones[ref_id];
                if(this->unused_bones.count(ref_id)) bone_id = this->unused_bones[ref_id];

                for(auto& tree : this->bone_trees) {
                    Model::Bone& bone = this->FindBone(bone_id, tree.second);
                    if(bone.index != UINT32_MAX) {
                        bone.animations[layer.name].translations = translations;
                        bone.animations[layer.name].rotations = rotations;
                        bone.animations[layer.name].scalings = scalings;
                    }
                }
            }
        }
    }

    std::array<std::vector<Model::KEYFRAME>, 3> FbxParser::ParseCurveNode(FBX_CURVE_NODE const& curve_node)
    {
        std::set<fbx_duration> key_times;
        std::array<std::vector<Model::KEYFRAME>, 3> keyframes;

        for(auto& curve : curve_node.curves)
            for(auto& key_time : curve.second.key_time)
                key_times.insert(key_time);

        for(auto& time : key_times) {
            std::chrono::milliseconds keyframe_time = std::chrono::duration_cast<std::chrono::milliseconds>(time);
            for(auto& curve : curve_node.curves) {
                for(uint32_t i=0; i<curve.second.key_time.size(); i++) {
                    auto key_time = curve.second.key_time[i];
                    auto key_value = curve.second.key_value[i];
                    if(key_time == time) {
                        if(curve.first == "d|X") keyframes[0].push_back({keyframe_time, key_value});
                        if(curve.first == "d|Y") keyframes[1].push_back({keyframe_time, key_value});
                        if(curve.first == "d|Z") keyframes[2].push_back({keyframe_time, key_value});
                        break;
                    }
                }
            }
        }

        for(auto& keycompnent : keyframes)
            if(keycompnent.size() == 2 && keycompnent[0].value == keycompnent[1].value)
                keycompnent.resize(1);

        return keyframes;
    }

    void FbxParser::ParseAnimationStack(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const animation_stack = this->GetObjectNode("AnimationStack", nodes);

        for(auto const& node : animation_stack) {
            fbx_duration animation_time;

            std::map<std::string, FbxParser::FBX_PROPERTY70> properties70 = this->ParseProperties70(node);
            if(properties70.count("LocalStop") > 0) animation_time = fbx_duration(DataPacker::Packer::ParseInt64(*this->data, properties70["LocalStop"].value[0].position + 1));

            for(auto& child : this->connections[node.id].children) {
                for(auto& layer : this->animation_layers) {
                    if(layer.id == child.id) {
                        layer.duration = std::chrono::duration_cast<std::chrono::milliseconds>(animation_time);

                        std::cout << "AnimationStack [" << layer.name << "] : ";
                        Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
                        std::cout << "OK" << std::endl;
                        Log::Terminal::SetTextColor();
                    }
                }
            }
        }
    }


    void FbxParser::ParseAnimationLayer(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const animation_layers = this->GetObjectNode("AnimationLayer", nodes);
        std::vector<FbxParser::FBX_NODE> const models = this->GetObjectNode("Model", nodes);
        std::vector<FbxParser::FBX_NODE> const deformers = this->GetObjectNode("Deformer", nodes);

        for(auto const& node : animation_layers) {
            FBX_ANIMATION_LAYER animation_layer;
            animation_layer.id = node.id;
            animation_layer.name = node.attribute_name;

            if(this->connections.count(node.id)) {
                for(auto& child : this->connections[node.id].children) {
                    for(auto& curve_node : this->curve_nodes) {
                        if(curve_node.id == child.id) {
                            if(this->connections.count(curve_node.id)) {
                                for(auto& parent : this->connections[curve_node.id].parents) {
                                    if(!parent.relationship.empty()) {

                                        for(auto& model : models) {
                                            if(model.id == parent.id) {

                                                if(animation_layer.animation_nodes.count(model.attribute_name) > 0) {
                                                    animation_layer.animation_nodes[model.attribute_name].curve_nodes[curve_node.attribute_name] = curve_node;

                                                }else{

                                                    FBX_ANIMATION_NODE anim_node = {};
                                                    anim_node.curve_nodes[curve_node.attribute_name] = curve_node;

                                                    anim_node.id = model.id;
                                                    anim_node.model = model;

                                                    for(auto& geometry : this->mesh_geometries) {
                                                        if(geometry.model.id == model.id) {
                                                            anim_node.transform = geometry.global_transformation;
                                                            break;
                                                        }
                                                    }

                                                    animation_layer.animation_nodes[model.attribute_name] = anim_node;
                                                }
                                                break;
                                            }
                                        }

                                        for(auto& deformer : deformers) {
                                            if(deformer.id == parent.id) {

                                                int64_t morper_id = this->connections[deformer.id].parents[0].id;
                                                int64_t geometry_id = this->connections[morper_id].parents[0].id;
                                                int64_t model_id = this->connections[geometry_id].parents[0].id;

                                                FBX_NODE model_node;
                                                for(auto& model : models)
                                                    if(model.id == model_id)
                                                        model_node = model;

                                                if(animation_layer.animation_nodes.count(deformer.attribute_name) > 0) {
                                                    animation_layer.animation_nodes[model_node.attribute_name].curve_nodes[curve_node.attribute_name] = curve_node;
                                            
                                                }else{
                                                    FBX_ANIMATION_NODE anim_node = {};
                                                    anim_node.model = model_node;

                                                    for(auto& morpher : deformers)
                                                        if(morpher.id == morper_id)
                                                            anim_node.morph_name = morpher.attribute_name;

                                                    animation_layer.animation_nodes[model_node.attribute_name] = anim_node;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            this->animation_layers.push_back(animation_layer);

            std::cout << "AnimationLayer [" << animation_layer.name << "] : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();
        }
    }

    void FbxParser::ParseAnimationCurve(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const anim_curves = this->GetObjectNode("AnimationCurve", nodes);

        for(auto const& node : anim_curves) {
            FBX_ANIMATION_CURVE anim_curve;
            anim_curve.id = node.id;

            anim_curve.key_value = this->ParseFloat32Array(FBX_PROPERTY_DATA(node.children,"KeyValueFloat"));

            auto key_times = this->ParseInt64Array(FBX_PROPERTY_DATA(node.children,"KeyTime"));
            for(auto& key_time : key_times)
                anim_curve.key_time.push_back(fbx_duration(key_time));

            if(this->connections.count(anim_curve.id)) {
                for(FBX_CURVE_NODE& curve_node : this->curve_nodes) {
                    if(curve_node.id == this->connections[anim_curve.id].parents[0].id) {
                        std::string relationship = this->connections[anim_curve.id].parents[0].relationship;
                        if(relationship == "d|X" || relationship == "d|Y" || relationship == "d|Z") curve_node.curves[relationship] = anim_curve;
                        if(relationship == "d|DeformPercent") curve_node.curves["DeformPercent"] = anim_curve;
                    }
                }
            }
        }

        if(this->curve_nodes.size() > 0) {
            std::cout << "AnimationCurve : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();
        }
    }

    void FbxParser::ParseAnimationCurveNode(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const anim_curve_nodes = this->GetObjectNode("AnimationCurveNode", nodes);

        for(auto const& node : anim_curve_nodes) {
            std::vector<std::string> search = {"R", "S", "T", "DeformPercent"};
            for(auto& key : search) {
                if(node.attribute_name == key) {
                    FBX_CURVE_NODE curve_node = {};
                    curve_node.id = node.id;
                    curve_node.attribute_name = node.attribute_name;

                    // Lecture des Properties70
                    std::map<std::string, FbxParser::FBX_PROPERTY70> properties70 = this->ParseProperties70(node);

                    if(properties70.count("d|X") > 0) curve_node.vector[0] = (float)DataPacker::Packer::ParseFloat64(*this->data, properties70["d|X"].value[0].position + 1);
                    if(properties70.count("d|Y") > 0) curve_node.vector[1] = (float)DataPacker::Packer::ParseFloat64(*this->data, properties70["d|Y"].value[0].position + 1);
                    if(properties70.count("d|Z") > 0) curve_node.vector[2] = (float)DataPacker::Packer::ParseFloat64(*this->data, properties70["d|Z"].value[0].position + 1);
                    if(properties70.count("d|DeformPercent") > 0) curve_node.morph = DataPacker::Packer::ParseFloat64(*this->data, properties70["d|DeformPercent"].value[0].position + 1);

                    this->curve_nodes.push_back(curve_node);
                }
            }
        }

        if(this->curve_nodes.size() > 0) {
            std::cout << "AnimationCurveNode[" << this->curve_nodes.size() << "] : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();
        }
    }

    void FbxParser::ParseSkeletons(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const& deformer_nodes = this->GetObjectNode("Deformer", nodes);
        std::vector<FbxParser::FBX_NODE> const& geometry_nodes = this->GetObjectNode("Geometry", nodes);
        std::vector<FbxParser::FBX_NODE> const& model_nodes = this->GetObjectNode("Model", nodes);

        for(auto deformer : deformer_nodes) {

            // On filtre les Skin
            if(deformer.attribute_type != "Skin") continue;

            FBX_SKELETON skeleton;
            skeleton.id = deformer.id;
            skeleton.name = deformer.attribute_name;

            for(auto const& parent : this->connections[skeleton.id].parents) {
                for(auto& geo : geometry_nodes) {
                    if(geo.id == parent.id) {
                        skeleton.mesh_name = geo.attribute_name;
                        break;
                    }
                }
            }

            for(auto child : this->connections[skeleton.id].children) {
                for(auto const& bone_node : deformer_nodes) {
                    if(bone_node.id == child.id) {
                        FBX_BONE bone;
                        bone.id = bone_node.id;
                        bone.name = bone_node.attribute_name;

                        std::vector<double> matrix = this->ParseFloat64Array(FBX_PROPERTY_DATA(bone_node.children,"TransformLink"));
                        for(uint32_t i=0; i<matrix.size(); i++) bone.transform_link[i] = static_cast<float>(matrix[i]);

                        matrix = this->ParseFloat64Array(FBX_PROPERTY_DATA(bone_node.children,"Transform"));
                        for(uint32_t i=0; i<matrix.size(); i++) bone.transform[i] = static_cast<float>(matrix[i]);

                        if(bone_node.children.count("Mode") > 0)
                            bone.link_mode = DataPacker::Packer::ParseInt32(*this->data, FBX_PROPERTY_DATA(bone_node.children,"Mode").position);

                        if(bone_node.children.count("Indexes") > 0) {
                            bone.indices = this->ParseInt32Array(FBX_PROPERTY_DATA(bone_node.children,"Indexes"));
                            bone.weights = this->ParseFloat64Array(FBX_PROPERTY_DATA(bone_node.children,"Weights"));
                        }

                        bone.bone_id = -1;
                        for(auto const& model : model_nodes) {
                            for(auto parent : this->connections[model.id].parents) {
                                if(bone.id == parent.id) {
                                    bone.bone_id = model.id;
                                    break;
                                }
                            }
                            if(bone.bone_id >= 0) break;
                        }

                        skeleton.bones.push_back(bone);
                    }
                }
            }

            if(skeleton.bones.size() > 0) {
                std::cout << "Squelette[" << skeleton.mesh_name << "] : ";
                Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
                std::cout << "OK" << std::endl;
                Log::Terminal::SetTextColor();
                this->skeletons.push_back(skeleton);
            }
        }
    }

    void FbxParser::SetBoneOffset(uint32_t bone_id, std::string const& mesh_name, Maths::Matrix4x4 const& offset, Model::Bone& bone_tree)
    {
        if(bone_tree.index == bone_id) {
            bone_tree.offsets[mesh_name] = offset;
            return;
        }

        for(Model::Bone& child : bone_tree.children)
            this->SetBoneOffset(bone_id, mesh_name, offset, child);
    }

    void FbxParser::ApplyBonesOffsets(FBX_MESH_GEOMETRY& mesh_geometry, std::string const& mesh_name)
    {
        if(mesh_geometry.skeleton.id >= 0) {
            for(auto& tree : this->bone_trees) {
                for(auto& cluster : mesh_geometry.skeleton.bones) {
                    Model::Bone bone = this->GetBone(cluster, tree.second);
                    this->SetBoneOffset(bone.index, mesh_name, cluster.transform, tree.second);
                }
            }
        }

        if(mesh_geometry.cluster.id >= 0) {
            for(auto& tree : this->bone_trees) {
                Model::Bone bone = this->GetBone(mesh_geometry.cluster, tree.second);
                this->SetBoneOffset(bone.index, mesh_name, mesh_geometry.local_transformation, tree.second);
            }
        }
    }

    /**
     * Lecture du node Object.Model
     */
    void FbxParser::ParseMeshes(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const model_nodes = this->GetObjectNode("Model", nodes);

        for(auto mesh : model_nodes) {

            // On filtre les Mesh
            if(mesh.attribute_type != "Mesh") continue;

            // On recherche la géométrie et les matériaux associés à ce mesh
            FBX_MESH_GEOMETRY mesh_geometry;
            mesh_geometry.id = -1;
            std::vector<FBX_MATERIAL*> mesh_materials;
            for(auto child : this->connections[mesh.id].children) {

                // Géométrie
                if(mesh_geometry.id < 0) {
                    for(auto& geometry : this->mesh_geometries) {
                        if(geometry.id == child.id) {
                            mesh_geometry = geometry;
                        }
                    }
                }

                // Matériaux
                for(FBX_MATERIAL& material : this->materials)
                    if(material.id == child.id)
                        mesh_materials.push_back(&material);
            }

            // Création du mesh serializé
            Model::Mesh serialized_mesh;
            serialized_mesh.name = mesh.attribute_name;

            // Malheureusement le culling est mal géré par le FBX
            /*if(mesh.children.count("Culling") > 0) {
                std::string culling = this->ParseString(FBX_PROPERTY_DATA(mesh.children,"Culling"));
                if(culling == "CullingOff") serialized_mesh.culling = Model::Mesh::BACKFACE_CULLING::CULLING_NONE;
                else if(culling == "CullingOnCCW") serialized_mesh.culling = Model::Mesh::BACKFACE_CULLING::CULLING_CLOCKWISE;
                else if(culling == "CullingOnCW") serialized_mesh.culling = Model::Mesh::BACKFACE_CULLING::CULLING_COUNTER_CLOCKWISE;
            }*/

            // Construction du vertex buffer
            serialized_mesh.vertex_buffer.resize(mesh_geometry.vertices.size() / 3);
            for(uint32_t i=0; i<serialized_mesh.vertex_buffer.size(); i++) {
                serialized_mesh.vertex_buffer[i] = {
                    static_cast<float>(mesh_geometry.vertices[i * 3 + 0]),
                    static_cast<float>(mesh_geometry.vertices[i * 3 + 1]),
                    static_cast<float>(mesh_geometry.vertices[i * 3 + 2])
                };
            }

            // Normal buffer
            if(!mesh_geometry.normal.buffer.empty())
                serialized_mesh.normal_buffer.resize(serialized_mesh.vertex_buffer.size());

            // Coordonnées de texture
            if(!mesh_geometry.uv.buffer.empty()) {
                serialized_mesh.uv_buffer.resize(mesh_geometry.uv.buffer.size() / 2);
                for(uint32_t i=0; i<serialized_mesh.uv_buffer.size(); i++) {
                    serialized_mesh.uv_buffer[i] = {
                        static_cast<float>(mesh_geometry.uv.buffer[i * 2 + 0]),
                        static_cast<float>(1.0f - mesh_geometry.uv.buffer[i * 2 + 1])
                    };
                }
            }

            // Le mesh est attaché à un seul matériau
            if(!mesh_geometry.material.indices.empty()
            && mesh_geometry.material.mapping_information_type == "AllSame"
            && mesh_geometry.material.indices.size() == 1
            && mesh_materials.size() > mesh_geometry.material.indices[0]) {
                auto& material = mesh_materials[mesh_geometry.material.indices[0]];
                serialized_mesh.materials.push_back(std::pair<std::string, std::vector<uint32_t>>(material->name, {}));
            }

            // Ajout du squelette
            if(mesh_geometry.skeleton.id >= 0) {
                serialized_mesh.skeleton = mesh_geometry.skeleton.name;
                this->ApplyBonesOffsets(mesh_geometry, serialized_mesh.name);
                serialized_mesh.deformers.resize(serialized_mesh.vertex_buffer.size());
                for(FBX_BONE cluster : mesh_geometry.skeleton.bones) {
                    for(uint32_t i=0; i<cluster.indices.size(); i++) {
                        if(cluster.weights[i] > 0.0f) {
                    
                            uint32_t current_bone_id = this->referenced_bones[cluster.bone_id];
                            uint32_t new_bone_id;
                            if(this->used_bones.count(current_bone_id)) {
                                new_bone_id = this->used_bones[current_bone_id];
                            }else{
                                new_bone_id = this->next_used_bone_id;
                                this->next_used_bone_id++;
                                this->used_bones[current_bone_id] = new_bone_id;
                            }

                            serialized_mesh.deformers[cluster.indices[i]].AddBone(static_cast<uint32_t>(new_bone_id), static_cast<float>(cluster.weights[i]));
                        }
                    }
                }
            }else if(mesh_geometry.cluster.id >= 0) {
                uint32_t bone_id = UINT32_MAX;
                if(this->referenced_bones.count(mesh_geometry.cluster.bone_id)) bone_id = this->referenced_bones[mesh_geometry.cluster.bone_id];

                uint32_t use_bone_id;
                if(!this->used_bones.count(bone_id)) {
                    use_bone_id = this->next_used_bone_id;
                    this->next_used_bone_id++;
                    this->used_bones[bone_id] = use_bone_id;
                }else{
                    use_bone_id = this->used_bones[bone_id];
                }

                for(auto& tree : this->bone_trees) {
                    auto bone = this->FindBone(bone_id, tree.second);
                    if(bone.index == bone_id) {
                        serialized_mesh.skeleton = tree.first;
                        this->ApplyBonesOffsets(mesh_geometry, serialized_mesh.name);
                        break;
                    }
                }

                serialized_mesh.deformers.resize(1);
                serialized_mesh.deformers[0].AddBone(use_bone_id, 1.0f);
            }

            ////////////////////////////////////
            // Passage des faces en triangles //
            //   Et refonte du index buffer   //
            ////////////////////////////////////

            std::vector<uint32_t> face_vertex;
            std::vector<uint32_t> face_uv;
            uint32_t fbx_face_index = 0;
            uint32_t output_face_index = 0;
            for(uint32_t i=0; i<mesh_geometry.vertex_indices.size(); i++) {

                // Lecture du index buffer FBX
                int32_t signed_vertex_index = mesh_geometry.vertex_indices[i];
                uint32_t unsigned_vertex_index = static_cast<uint32_t>(signed_vertex_index);
                if(signed_vertex_index < 0) unsigned_vertex_index = ~unsigned_vertex_index;
                uint32_t vertex_offset = unsigned_vertex_index * 3;
                face_vertex.push_back(unsigned_vertex_index);
                if(!mesh_geometry.uv.indices.empty()) face_uv.push_back(mesh_geometry.uv.indices[i]);

                // Chaque indice négatif indique la cloture d'une face
                if(signed_vertex_index < 0) {
                
                    // Construction de plusieurs triangles à partir de la face
                    std::vector<uint32_t> current_face_indices;
                    for(uint8_t j=0; j<face_vertex.size() - 2; j++) {

                        // Triangle des sommets
                        serialized_mesh.index_buffer.push_back(face_vertex[0]);
                        serialized_mesh.index_buffer.push_back(face_vertex[j+1]);
                        serialized_mesh.index_buffer.push_back(face_vertex[j+2]);

                        // Triangle des UV
                        if(!mesh_geometry.uv.indices.empty()) {
                            serialized_mesh.uv_index.push_back(face_uv[0]);
                            serialized_mesh.uv_index.push_back(face_uv[j+1]);
                            serialized_mesh.uv_index.push_back(face_uv[j+2]);
                        }

                        current_face_indices.push_back(output_face_index);
                        output_face_index++;
                    }
                    face_vertex.clear();
                    face_uv.clear();

                    // Lecture des matériaux associés à des faces
                    if(!mesh_geometry.material.indices.empty() && mesh_geometry.material.mapping_information_type == "ByPolygon") {

                        if(fbx_face_index > mesh_geometry.material.indices.size()) {
                            std::cout << "Mesh [" << serialized_mesh.name << "] (geometry : " << mesh_geometry.name << ") : ";
                            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::ORANGE);
                            std::cout << "Material face index out of bounds" << std::endl;
                            Log::Terminal::SetTextColor();
                        }else{
                            uint32_t material_index = mesh_geometry.material.indices[fbx_face_index];
                            if(material_index > mesh_materials.size()) {
                                std::cout << "Mesh [" << serialized_mesh.name << "] (geometry : " << mesh_geometry.name << ") : ";
                                Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::ORANGE);
                                std::cout << "Face Material out of bounds" << std::endl;
                                Log::Terminal::SetTextColor();
                            }else{
                                std::string material_name = mesh_materials[material_index]->name;
                                bool found = false;
                                for(auto& mesh_material : serialized_mesh.materials) {
                                    if(mesh_material.first == material_name) {
                                        mesh_material.second.insert(mesh_material.second.end(), current_face_indices.begin(), current_face_indices.end());
                                        found = true;
                                        break;
                                    }
                                }
                                if(!found) serialized_mesh.materials.push_back(std::pair<std::string, std::vector<uint32_t>>(material_name, current_face_indices));
                            }
                        }
                    }

                    fbx_face_index++;
                }

                // Construction des normales
                if(!mesh_geometry.normal.buffer.empty()) {

                    int32_t normal_buffer_index;
                    if(mesh_geometry.normal.reference_information_type == "Direct") normal_buffer_index = i * 3;
                    else normal_buffer_index = mesh_geometry.normal.indices[i] * 3;

                    Maths::Vector3 normal = {
                        static_cast<float>(mesh_geometry.normal.buffer[normal_buffer_index + 0]),
                        static_cast<float>(mesh_geometry.normal.buffer[normal_buffer_index + 1]),
                        static_cast<float>(mesh_geometry.normal.buffer[normal_buffer_index + 2])
                    };

                    serialized_mesh.normal_buffer[unsigned_vertex_index] = serialized_mesh.normal_buffer[unsigned_vertex_index] + normal;
                }
            }

            // Normalisation des normales
            for(auto& normal : serialized_mesh.normal_buffer)
                normal = normal.Normalize();

            // Un mesh qui n'a pas de texture n'a pas besoin d'UV
            bool has_texture = false;
            for(auto& mesh_material : serialized_mesh.materials) {
                for(auto& engine_material : this->materials) {
                    if(mesh_material.first == engine_material.name && !engine_material.textures.empty()) {
                        has_texture = true;
                        break;
                    }
                }
                if(has_texture) break;
            }
            if(!has_texture) {
                serialized_mesh.uv_buffer.clear();
                serialized_mesh.uv_index.clear();
            }

            std::cout << "Mesh [" << serialized_mesh.name << "] (geometry : " << mesh_geometry.name << ") : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();

            // Le mesh parsé est ajouté à la liste finale
            this->meshes.push_back(serialized_mesh);
        }
    }

    FbxParser::FBX_BONE FbxParser::FindCluster(int64_t id)
    {
        for(auto& skeleton : this->skeletons)
            for(auto& cluster : skeleton.bones)
                if(cluster.bone_id == id)
                    return cluster;
        return {};
    }

    Model::Bone FbxParser::GetBone(FBX_BONE const& node, Model::Bone const& bone_tree)
    {
        uint32_t bone_id = this->referenced_bones[node.bone_id];

        if(bone_tree.index == bone_id) return bone_tree;

        for(auto& child : bone_tree.children) {
            Model::Bone bone = this->GetBone(node, child);
            if(bone.index != UINT32_MAX) return bone;
        }

        return {};
    }

    Model::Bone FbxParser::GetParentBone(Model::Bone const& bone, Model::Bone const& bone_tree)
    {
        for(auto& child : bone_tree.children) {
            if(child.index == bone.index) return bone_tree;
            Model::Bone search = GetParentBone(bone, child);
            if(search.index != UINT32_MAX) return search;
        }

        return {};
    }

    Maths::Matrix4x4 FbxParser::GetBoneGlobalTransform(Model::Bone const& bone, Model::Bone const& bone_tree)
    {
        Model::Bone parent = this->GetParentBone(bone, bone_tree);
        if(parent.index == UINT32_MAX) return bone.transformation;
        return this->GetBoneGlobalTransform(parent, bone_tree) * bone.transformation;
    }

    void FbxParser::ComputeClusterDeformation(FBX_MESH_GEOMETRY& mesh_geometry)
    {
        for(auto& cluster : mesh_geometry.skeleton.bones) {
            for(auto& tree : this->bone_trees) {
                Model::Bone bone = this->GetBone(cluster, tree.second);
                cluster.deformation = this->GetBoneGlobalTransform(bone, tree.second) * cluster.transform;
            }
        }
    }

    /**
     * Lecture du node Object.Geometry
     */
    void FbxParser::ParseMeshGeometry(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const geometry_nodes = this->GetObjectNode("Geometry", nodes);
        std::vector<FbxParser::FBX_NODE> const models = this->GetObjectNode("Model", nodes);

        for(auto geometry : geometry_nodes) {
            if(geometry.attribute_type != "Mesh") continue;

            FBX_MESH_GEOMETRY mesh_geometry;
            mesh_geometry.id = geometry.id;
            mesh_geometry.name = geometry.attribute_name;
            mesh_geometry.vertices = this->ParseFloat64Array(geometry.children["Vertices"][0].properties[0]);
            mesh_geometry.vertex_indices = this->ParseInt32Array(geometry.children["PolygonVertexIndex"][0].properties[0]);

            if(geometry.children.count("LayerElementUV") > 0) mesh_geometry.uv = this->ParseLayerElement(geometry.children["LayerElementUV"][0], "UV", "UVIndex");
            if(geometry.children.count("LayerElementColor") > 0) mesh_geometry.color = this->ParseLayerElement(geometry.children["LayerElementColor"][0], "Colors", "ColorIndex");
            if(geometry.children.count("LayerElementNormal") > 0) mesh_geometry.normal = this->ParseLayerElement(geometry.children["LayerElementNormal"][0], "Normals", "NormalIndex");
            if(geometry.children.count("LayerElementMaterial") > 0) mesh_geometry.material = this->ParseLayerElement(geometry.children["LayerElementMaterial"][0], std::string(), "Materials");
            if(geometry.children.count("Layer") > 0) mesh_geometry.layers = this->ParseLayers(geometry.children["Layer"][0]);

            mesh_geometry.model = {};
            for(auto parent : this->connections[mesh_geometry.id].parents) {
                for(FBX_NODE const& model : models) {
                    if(model.id == parent.id) {
                        mesh_geometry.model = model;
                        break;
                    }
                }
                if(mesh_geometry.model.id >= 0) break;
            }
            if(mesh_geometry.model.id < 0) return;

            for(auto parent : this->connections[mesh_geometry.model.id].parents) {
                for(FBX_NODE const& model : models) {
                    if(model.id == parent.id) {
                        mesh_geometry.cluster = this->FindCluster(model.id);
                        auto skeleton = this->GetRootModel(model, models);
                        for(FBX_SKELETON& item : this->skeletons) if(item.id == skeleton.id) mesh_geometry.skeleton = item;
                        break;
                    }
                }
                if(mesh_geometry.cluster.id >= 0) break;
            }

            for(auto& child : this->connections[mesh_geometry.id].children) {
                for(FBX_SKELETON& item : this->skeletons) if(item.id == child.id) mesh_geometry.skeleton = item;
                // for(FBX_BLENSHAPE& item : this->blend_shapes) if(item.id == child.id) mesh_geometry.blend_shape = item;
            }

            if(mesh_geometry.skeleton.id >= 0) {
                std::vector<int64_t> remove_me;
                for(auto& bone : mesh_geometry.skeleton.bones)
                    if(bone.indices.empty() && bone.weights.empty())
                        remove_me.push_back(bone.id);
                while(!remove_me.empty()) {
                    for(auto it = mesh_geometry.skeleton.bones.begin(); it != mesh_geometry.skeleton.bones.end(); it++) {
                        if(it->id == remove_me[remove_me.size() - 1]) {
                            remove_me.pop_back();
                            mesh_geometry.skeleton.bones.erase(it);
                            break;
                        }
                    }
                }
            }

            mesh_geometry.global_transformation = this->ComputeTransformation(mesh_geometry.model, models);
            Maths::Matrix4x4 parent_transformation = IDENTITY_MATRIX;
            mesh_geometry.local_transformation = this->ComputeTransformation(mesh_geometry.model, parent_transformation);
            this->ComputeClusterDeformation(mesh_geometry);

            std::cout << "Mesh Geometry[" << mesh_geometry.name << "] (" << mesh_geometry.vertices.size() << " vertices) : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();

            this->mesh_geometries.push_back(mesh_geometry);
        }
    }

    std::vector<FbxParser::FBX_LAYER> FbxParser::ParseLayers(FBX_NODE const& layer_node)
    {
        std::vector<FBX_LAYER> layers;
        for(auto& element : layer_node.children.at("LayerElement")) {
            FBX_LAYER layer;
            layer.Type = this->ParseString(FBX_PROPERTY_DATA(element.children,"Type"));
            layer.TypedIndex = DataPacker::Packer::ParseUint32(*this->data, FBX_PROPERTY_DATA(element.children,"TypedIndex").position);
            layers.push_back(layer);
        }

        return layers;
    }

    /**
     * Un objet de type geometry contient des des propriétés au format LAYER_ELEMENT,
     * cette fonction sert à parser ces propriétés
     */
    FbxParser::FBX_LAYER_ELEMENT FbxParser::ParseLayerElement(FBX_NODE const& layer_node, std::string const& buffer_name, std::string const& indices_name)
    {
        FBX_LAYER_ELEMENT layer_element;
        layer_element.mapping_information_type = this->ParseString(FBX_PROPERTY_DATA(layer_node.children,"MappingInformationType"));
        layer_element.reference_information_type = this->ParseString(FBX_PROPERTY_DATA(layer_node.children,"ReferenceInformationType"));

        if(!buffer_name.empty() && layer_node.children.count(buffer_name) > 0) layer_element.buffer = this->ParseFloat64Array(layer_node.children.at(buffer_name).at(0).properties.at(0));

        if(layer_element.reference_information_type == "IndexToDirect" && !indices_name.empty() && layer_node.children.count(indices_name) > 0)
            layer_element.indices = this->ParseInt32Array(layer_node.children.at(indices_name).at(0).properties.at(0));

        return layer_element;
    }

    /**
     * Lecture du node Object.Textures
     */
    void FbxParser::ParseMaterials(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const material_nodes = this->GetObjectNode("Material", nodes);

        // Chaque matériau dispose d'un id qui fera le lien avec les mesh auxquels il est appliqué
        this->next_material_id = 0;

        for(auto node : material_nodes) {
            FBX_MATERIAL material;
            material.id = node.id;
            material.name = node.attribute_name;
            material.unique_id = this->next_material_id;
            this->next_material_id++;

            if(node.children.count("ShadingModel") > 0) material.ShadingModel = this->ParseString(FBX_PROPERTY_DATA(node.children,"ShadingModel"));

            // Lecture des Properties70
            std::map<std::string, FbxParser::FBX_PROPERTY70> properties70 = this->ParseProperties70(node);

            // Lecture des couleurs de matériaux
            if(properties70.count("EmissiveColor") > 0) material.EmissiveColor = this->ParseColor(properties70["EmissiveColor"]);
            if(properties70.count("AmbientColor") > 0) material.AmbientColor = this->ParseColor(properties70["AmbientColor"]);
            if(properties70.count("DiffuseColor") > 0) material.DiffuseColor = this->ParseColor(properties70["DiffuseColor"]);
            if(properties70.count("SpecularColor") > 0) material.SpecularColor = this->ParseColor(properties70["SpecularColor"]);
            if(properties70.count("Bump") > 0) material.Bump = this->ParseColor(properties70["Bump"]);
            if(properties70.count("TransparentColor") > 0) material.TransparentColor = this->ParseColor(properties70["TransparentColor"]);

            // Lecture des facteurs de matériaux
            if(properties70.count("EmissiveFactor") > 0) material.EmissiveFactor = DataPacker::Packer::ParseFloat64(*this->data, properties70["EmissiveFactor"].value[0].position + 1);
            if(properties70.count("AmbientFactor") > 0) material.AmbientFactor = DataPacker::Packer::ParseFloat64(*this->data, properties70["AmbientFactor"].value[0].position + 1);
            if(properties70.count("DiffuseFactor") > 0) material.DiffuseFactor = DataPacker::Packer::ParseFloat64(*this->data, properties70["DiffuseFactor"].value[0].position + 1);
            if(properties70.count("SpecularFactor") > 0) material.SpecularFactor = DataPacker::Packer::ParseFloat64(*this->data, properties70["SpecularFactor"].value[0].position + 1);
            if(properties70.count("BumpFactor") > 0) material.BumpFactor = DataPacker::Packer::ParseFloat64(*this->data, properties70["BumpFactor"].value[0].position + 1);
            if(properties70.count("TransparencyFactor") > 0) material.TransparencyFactor = DataPacker::Packer::ParseFloat64(*this->data, properties70["TransparencyFactor"].value[0].position + 1);
            if(properties70.count("Shininess") > 0) material.Shininess = DataPacker::Packer::ParseFloat64(*this->data, properties70["Shininess"].value[0].position + 1);

            // Lecture des textures associées
            for(auto child : this->connections[material.id].children) {
                if(!child.relationship.empty()) {
                    for(FBX_TEXTURE& texture : this->textures) {
                        if(texture.id == child.id) material.textures[child.relationship] = &texture;
                    }
                }
            }

            Model::Mesh::MATERIAL engine_material;
            engine_material.ambient = {
                static_cast<float>(material.AmbientColor[0]),
                static_cast<float>(material.AmbientColor[1]),
                static_cast<float>(material.AmbientColor[2]),
                static_cast<float>(material.AmbientFactor)
            };
            engine_material.diffuse = {
                static_cast<float>(material.DiffuseColor[0]),
                static_cast<float>(material.DiffuseColor[1]),
                static_cast<float>(material.DiffuseColor[2]),
                static_cast<float>(material.DiffuseFactor)
            };
            engine_material.specular = {
                static_cast<float>(material.SpecularColor[0]),
                static_cast<float>(material.SpecularColor[1]),
                static_cast<float>(material.SpecularColor[2]),
                static_cast<float>(material.SpecularFactor)
            };
            engine_material.transparency = static_cast<float>(material.TransparencyFactor);

            if(material.textures.count("DiffuseColor") > 0
                && material.textures["DiffuseColor"] != nullptr
                && !material.textures["DiffuseColor"]->RelativeFilename.empty())
                engine_material.texture = Tools::GetFileName(material.textures["DiffuseColor"]->RelativeFilename);

            this->engine_materials[material.name] = engine_material;

            std::cout << "Material[" << material.name << "] : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();

            this->materials.push_back(material);
        }
    }

    /**
     * Lecture d'une property70 de type "Color" ou "Vector"
     * composé de 3 doubles (RGB ou XYZ)
     */
    std::array<double, 3> FbxParser::ParseColor(FBX_PROPERTY70 const& property70)
    {
        std::array<double, 3> color = {};
        if(property70.value.size() != 3) return color;

        color[0] = DataPacker::Packer::ParseFloat64(*this->data, property70.value[0].position + 1);
        color[1] = DataPacker::Packer::ParseFloat64(*this->data, property70.value[1].position + 1);
        color[2] = DataPacker::Packer::ParseFloat64(*this->data, property70.value[2].position + 1);

        return color;
    }

    /**
     * Lecture des proprties70 d'un node
     */
    std::map<std::string, FbxParser::FBX_PROPERTY70> FbxParser::ParseProperties70(FBX_NODE const& node)
    {
        if(node.children.count("Properties70") == 0) return {};

        std::map<std::string, FBX_PROPERTY70> properties70;
        for(auto& property_node : node.children.at("Properties70").at(0).children.at("P")) {
            FBX_PROPERTY70 property70;
            property70.name = this->ParseString(property_node.properties[0]);
            property70.type1 = this->ParseString(property_node.properties[1]);
            property70.type2 = this->ParseString(property_node.properties[2]);
            property70.flag = this->ParseString(property_node.properties[3]);

            if(property_node.properties.size() > 4) property70.value.push_back(property_node.properties[4]);

            if(property70.name.substr(0, 4) == "Lcl ") property70.name[3] = '_';
            if(property70.type1.substr(0, 4) == "Lcl ") property70.type1[3] = '_';

            if(property70.type1 == "Color" || property70.type1 == "ColorRGB" || property70.type1 == "Vector" || property70.type1 == "Vector3D" || property70.type1.substr(0, 4) == "Lcl_") {
                property70.value.push_back(property_node.properties[5]);
                property70.value.push_back(property_node.properties[6]);
            }

            properties70[property70.name] = property70;
        }

        return properties70;
    }

    /**
     * Lecture du node Object.Textures
     */
    void FbxParser::ParseTextures(std::vector<FBX_NODE> const& nodes)
    {
        std::vector<FbxParser::FBX_NODE> const texture_nodes = this->GetObjectNode("Texture", nodes);

        for(auto node : texture_nodes) {
            FBX_TEXTURE texture;
            texture.id = node.id;
            texture.name = node.attribute_name;
        
            if(node.children.count("RelativeFilename") > 0) texture.RelativeFilename = this->ParseString(FBX_PROPERTY_DATA(node.children,"RelativeFilename"));
            std::replace(texture.RelativeFilename.begin(), texture.RelativeFilename.end(), '\\', '/');
            this->textures.push_back(texture);

            std::cout << "Texture[" << texture.RelativeFilename << "] : ";
            Log::Terminal::SetTextColor(Log::Terminal::TEXT_COLOR::GREEN);
            std::cout << "OK" << std::endl;
            Log::Terminal::SetTextColor();
        }
    }

    Maths::Matrix4x4 FbxParser::ComputeTransformation(FBX_NODE const& node, std::vector<FBX_NODE> const& models)
    {
        FBX_NODE const* parent_node = nullptr;
        for(auto parent : this->connections[node.id].parents) {
            for(FBX_NODE const& model : models) {
                if(model.id == parent.id) {
                    parent_node = &model;
                    break;
                }
            }
            if(parent_node != nullptr) break;
        }

        Maths::Matrix4x4 parent_transformation = IDENTITY_MATRIX;
        if(parent_node != nullptr)
            parent_transformation = this->ComputeTransformation(*parent_node, models);
        
        return this->ComputeTransformation(node, parent_transformation);
    }

    Maths::Matrix4x4 FbxParser::ComputeTransformation(FBX_NODE const& node, Maths::Matrix4x4 const& parent_transformation)
    {
        Maths::Matrix4x4::EULER_ORDER euler_order = Maths::Matrix4x4::EULER_ORDER::ZYX;
        Maths::Matrix4x4 lTranslationM;
	    Maths::Matrix4x4 lPreRotationM;
	    Maths::Matrix4x4 lRotationM;
	    Maths::Matrix4x4 lPostRotationM;

	    Maths::Matrix4x4 lScalingM;
	    Maths::Matrix4x4 lScalingPivotM;
	    Maths::Matrix4x4 lScalingOffsetM;
	    Maths::Matrix4x4 lRotationOffsetM;
	    Maths::Matrix4x4 lRotationPivotM;
	
	    Maths::Matrix4x4 lGlobalT;

        // Lecture des Properties70
        std::map<std::string, FbxParser::FBX_PROPERTY70> properties70 = this->ParseProperties70(node);

        // Lecture des couleurs de matériaux
        // if(properties70.count("EmissiveColor") > 0) material.EmissiveColor = this->ParseColor(properties70["EmissiveColor"]);

        if(properties70.count("RotationOrder") > 0) {
            std::string rotation_order = DataPacker::Packer::ParseString(
                *this->data,
                static_cast<uint32_t>(properties70["RotationOrder"].value[0].position),
                static_cast<uint32_t>(properties70["RotationOrder"].value[0].size)
            );
            if(rotation_order == "ZYX") euler_order = Maths::Matrix4x4::EULER_ORDER::ZYX;
            if(rotation_order == "YZX") euler_order = Maths::Matrix4x4::EULER_ORDER::YZX;
            if(rotation_order == "XZY") euler_order = Maths::Matrix4x4::EULER_ORDER::XZY;
            if(rotation_order == "ZXY") euler_order = Maths::Matrix4x4::EULER_ORDER::ZXY;
            if(rotation_order == "YXZ") euler_order = Maths::Matrix4x4::EULER_ORDER::YXZ;
            if(rotation_order == "XYZ") euler_order = Maths::Matrix4x4::EULER_ORDER::XYZ;
        }

        if(properties70.count("Lcl_Translation") > 0) lTranslationM = Maths::Matrix4x4::TranslationMatrix(Maths::Vector3(this->ParseVector(properties70["Lcl_Translation"])));

        if(properties70.count("PreRotation") > 0) {
            Maths::Vector3 vector(this->ParseVector(properties70["PreRotation"]));
            lPreRotationM = Maths::Matrix4x4::EulerRotation(lPreRotationM, vector.ToRadians(), euler_order);
        }

        if(properties70.count("Lcl_Rotation") > 0) {
            Maths::Vector3 vector(this->ParseVector(properties70["Lcl_Rotation"]));
            lPreRotationM = Maths::Matrix4x4::EulerRotation(lPreRotationM, vector.ToRadians(), euler_order);
        }

        if(properties70.count("PostRotation") > 0) {
            Maths::Vector3 vector(this->ParseVector(properties70["PostRotation"]));
            lPreRotationM = Maths::Matrix4x4::EulerRotation(lPreRotationM, vector.ToRadians(), euler_order);
        }

        if(properties70.count("Lcl_Scaling") > 0) {
            Maths::Vector3 scale(this->ParseVector(properties70["Lcl_Scaling"]));
            lScalingM = Maths::Matrix4x4::ScalingMatrix(scale);
        }

        // Pivots and offsets
        if(properties70.count("ScalingOffset") > 0) lScalingOffsetM = Maths::Matrix4x4::TranslationMatrix(Maths::Vector3(this->ParseVector(properties70["ScalingOffset"])));
        if(properties70.count("ScalingPivot") > 0) lScalingPivotM = Maths::Matrix4x4::TranslationMatrix(Maths::Vector3(this->ParseVector(properties70["ScalingPivot"])));
        if(properties70.count("RotationOffset") > 0) lRotationOffsetM = Maths::Matrix4x4::TranslationMatrix(Maths::Vector3(this->ParseVector(properties70["RotationOffset"])));
        if(properties70.count("RotationPivot") > 0) lRotationPivotM = Maths::Matrix4x4::TranslationMatrix(Maths::Vector3(this->ParseVector(properties70["RotationPivot"])));

        // parent transform
	    Maths::Matrix4x4 lParentGX(parent_transformation);

        // Global Rotation
	    Maths::Matrix4x4 lLRM = lPreRotationM * lRotationM * lPostRotationM;
	    Maths::Matrix4x4 lParentGRM = lParentGX.ExtractRotation();

        // Global Shear * Scaling
	    Maths::Matrix4x4 lParentTM;
	    Maths::Matrix4x4 lLSM = lScalingM;
	    Maths::Matrix4x4 lParentGSM;
	    Maths::Matrix4x4 lParentGRSM;

        //CopyTranslation(lParentGX, lParentTM);
        lParentTM.SetTranslation(lParentGX.GetTranslation());
	    lParentGRSM = lParentTM.Inverse() * lParentGX;
	    lParentGSM = lParentGRM.Inverse() * lParentGRSM;

        Maths::Matrix4x4 lGlobalRS;

        uint32_t inherit_type = 0;
        if(properties70.count("InheritType") > 0)
            inherit_type = DataPacker::Packer::ParseUint32(*this->data, properties70["InheritType"].value[0].position + 1);

	    if(inherit_type == 0) {
		    lGlobalRS = lParentGRM * lLRM * lParentGSM *  lLSM;

	    }else if(inherit_type == 1) {
		    lGlobalRS = lParentGRM * lParentGSM * lLRM * lLSM;

	    }else{
		    Maths::Matrix4x4 lParentGSM_noLocal = lParentGSM * lScalingM.Inverse();
		    lGlobalRS = lParentGRM * lLRM * lParentGSM_noLocal * lLSM;
	    }

        // Calculate the local transform matrix
	    Maths::Matrix4x4 lTransform = lTranslationM * lRotationOffsetM * lRotationPivotM * lPreRotationM * lRotationM * lPostRotationM * lRotationPivotM.Inverse() * lScalingOffsetM * lScalingPivotM * lScalingM * lScalingPivotM.Inverse();

	    Maths::Matrix4x4 lLocalTWithAllPivotAndOffsetInfo = lTransform.ToTranslationMatrix();

	    Maths::Matrix4x4 lGlobalTranslation = lParentGX * lLocalTWithAllPivotAndOffsetInfo;
        lGlobalT.SetTranslation(lGlobalTranslation.GetTranslation());

	    lTransform = lGlobalT * lGlobalRS;

	    return lTransform;
    }

    /**
     * Récupère une référence sur un noeud d'objet
     */
    std::vector<FbxParser::FBX_NODE> const FbxParser::GetObjectNode(std::string const& object_name, std::vector<FBX_NODE> const& nodes)
    {
        FBX_NODE const& objects = this->GetMainNode("Objects", nodes);

        for(auto const& object : objects.children)
            if(object.first == object_name)
                return object.second;

        return {};
    }

    /**
     * Récupère une référence sur un noeud principal
     */
    FbxParser::FBX_NODE const FbxParser::GetMainNode(std::string const& node_name, std::vector<FBX_NODE> const& nodes)
    {
        for(auto const& node : nodes)
            if(node.name == node_name)
                return node;

        return {};
    }

    /**
     * Établie toutes les relations entre les noeuds
     */
    void FbxParser::ParseConnections(std::vector<FBX_NODE> const& nodes)
    {
        FBX_NODE connections_node = this->GetMainNode("Connections", nodes);

        for(auto& node : connections_node.children["C"]) {
            FBX_RELATION child, parent;
            parent.id = DataPacker::Packer::ParseInt64(*this->data, node.properties[1].position + 1);
            child.id = DataPacker::Packer::ParseInt64(*this->data, node.properties[2].position + 1);

            if(node.properties.size() > 3) {
                child.relationship = this->ParseString(node.properties[3]);
                parent.relationship = child.relationship;
            }

            this->connections[child.id].children.push_back(parent);
            this->connections[parent.id].parents.push_back(child);
        }

        // std::vector<FbxParser::FBX_NODE> const& model_nodes = this->GetObjectNode("Model", nodes);
        // this->BuildConnectionTree(model_nodes);
    }

    /**
     * Vérifie la présence de l'emprunte FBX au début des données
     */
    bool FbxParser::isBinaryFbx()
    {
        std::string file_tag = "Kaydara FBX Binary  \0";
        return this->data->size() >= file_tag.size() && file_tag == std::string(this->data->begin(), this->data->begin() + file_tag.size());
    }

    /**
     * Récupère la version du FBX
     */
    uint32_t FbxParser::GetVersion()
    {
        return DataPacker::Packer::ParseUint32(*this->data, 23);
    }

    /**
    * Indique si la position correspond à la fin des données utiles du fichier
    */
    bool FbxParser::EndOfContent(size_t position)
    {
        if(this->data->size() % 16 == 0) return ((position + 160 + 16) & ~ 0xf) >= this->data->size();
        else return position + 160 + 16 >= this->data->size();
    }

    FbxParser::FBX_NODE FbxParser::ParseNode(size_t& position)
    {
        FBX_NODE node;

        // Lecture du nombre de propriétés ainsi que leur taille dans le fichier
        uint64_t end_offset, num_properties, property_list_length;
        if(this->version > 7500) {

            end_offset = DataPacker::Packer::ParseUint64(*this->data, position);
            position += sizeof(uint64_t);

            num_properties = DataPacker::Packer::ParseUint64(*this->data, position);
            position += sizeof(uint64_t);

            property_list_length = DataPacker::Packer::ParseUint64(*this->data, position);
            position += sizeof(uint64_t);
        }else{

            end_offset = static_cast<uint64_t>(DataPacker::Packer::ParseUint32(*this->data, position));
            position += sizeof(uint32_t);

            num_properties = static_cast<uint64_t>(DataPacker::Packer::ParseUint32(*this->data, position));
            position += sizeof(uint32_t);

            property_list_length = static_cast<uint64_t>(DataPacker::Packer::ParseUint32(*this->data, position));
            position += sizeof(uint32_t);
        }

        // Lecture du nom
        uint8_t name_length = static_cast<uint8_t>((*this->data)[position]);
        position += sizeof(uint8_t);
        node.name = DataPacker::Packer::ParseString(*this->data, static_cast<uint32_t>(position), name_length);
        position += name_length;

        // Noeud vide, on sort
        if(end_offset == 0) return node;

        // Lecture des propriétés
        for(uint64_t i=0; i<num_properties; i++) {
            FBX_PROPERTY property = this->ParseRawProperty(position);
            position += property.size;
            node.properties.push_back(property);
        }

        // ID du node
        if(node.properties.size() > 0) {
            switch(node.properties[0].type) {
                case 'Y' : node.id = static_cast<int64_t>(DataPacker::Packer::ParseInt16(*this->data, node.properties[0].position + 1)); break;
                case 'I' : node.id = static_cast<int64_t>(DataPacker::Packer::ParseInt32(*this->data, node.properties[0].position + 1)); break;
                case 'L' : node.id = DataPacker::Packer::ParseInt64(*this->data, node.properties[0].position + 1); break;
                default : node.id = UINT64_MAX;
            }
        }

        // Attribute Name
        if(node.properties.size() > 1 && node.properties[1].type == 'S')
            node.attribute_name = std::string(this->ParseString(node.properties[1]).c_str());

        // Attribute Type
        if(node.properties.size() > 2 && node.properties[2].type == 'S')
            node.attribute_type = std::string(this->ParseString(node.properties[2]).c_str());

        while(position < end_offset) {
            FBX_NODE sub_node = this->ParseNode(position);
            if(!this->IsNullNode(sub_node)) node.children[sub_node.name].push_back(sub_node);
        }

        return node;
    }

    FbxParser::FBX_PROPERTY FbxParser::ParseRawProperty(size_t position)
    {
        char type = (*this->data)[position];
        size_t offset = sizeof(char);

        switch(type) {
            case 'C' : return {type, position, static_cast<size_t>(sizeof(char) + offset)};
            case 'D' : return {type, position, static_cast<size_t>(sizeof(double) + offset)};
            case 'F' : return {type, position, static_cast<size_t>(sizeof(float) + offset)};
            case 'I' : return {type, position, static_cast<size_t>(sizeof(int32_t) + offset)};
            case 'L' : return {type, position, static_cast<size_t>(sizeof(int64_t) + offset)};
            case 'R' : 
            case 'S' : return {type, position, static_cast<size_t>(DataPacker::Packer::ParseUint32(*this->data, position + offset) + sizeof(uint32_t) + offset)};
            case 'Y' : return {type, position, static_cast<size_t>(sizeof(int16_t) + offset)};
            case 'b' :
            case 'c' :
            case 'd' :
            case 'e' :
            case 'f' :
            case 'i' :
            case 'l' :
            {
                uint32_t array_length = DataPacker::Packer::ParseUint32(*this->data, position + offset);
                offset += sizeof(uint32_t);

                uint32_t encoding = DataPacker::Packer::ParseUint32(*this->data, position + offset);
                offset += sizeof(uint32_t);

                uint32_t compressed_length = DataPacker::Packer::ParseUint32(*this->data, position + offset);
                offset += sizeof(uint32_t);

                if(encoding == ZLIB_COMPRESSION) return {type, position, static_cast<size_t>(compressed_length + offset)};

                if(encoding == NO_ENCODING) {
                    switch(type) {
                        case 'b' : 
                        case 'c' : return {type, position, static_cast<size_t>(sizeof(bool) * array_length + offset)};
                        case 'd' : return {type, position, static_cast<size_t>(sizeof(double) * array_length + offset)};
                        case 'f' : return {type, position, static_cast<size_t>(sizeof(float) * array_length + offset)};
                        case 'i' : return {type, position, static_cast<size_t>(sizeof(int32_t) * array_length + offset)};
                        case 'l' : return {type, position, static_cast<size_t>(sizeof(int64_t) * array_length + offset)};
                    }
                }
            }
            default : return {type, position, 1};
        }
    }

    /**
    * Détermine si un noeud ne contient ni propriétés, ni attributs, ni enfants, ni connecions ...
    */
    bool FbxParser::IsNullNode(FBX_NODE const& node)
    {
        return  node.id == -1 
                && node.name.empty()
                && node.attribute_name.empty()
                && node.attribute_type.empty()
                && node.children.size() == 0
                && node.properties.size() == 0;
    }

    /**
    * Lecture d'un buffer de double
    */
    std::vector<double> FbxParser::ParseFloat64Array(FBX_PROPERTY const& property)
    {
        size_t offset = sizeof(char);

        uint32_t array_length = DataPacker::Packer::ParseUint32(*this->data, property.position + offset);
        offset += sizeof(uint32_t);

        uint32_t encoding = DataPacker::Packer::ParseUint32(*this->data, property.position + offset);
        offset += sizeof(uint32_t);

        uint32_t compressed_length = DataPacker::Packer::ParseUint32(*this->data, property.position + offset);
        offset += sizeof(uint32_t);

        std::vector<char> data;

        if(encoding == ZLIB_COMPRESSION) {
            data = std::vector<char>(this->data->begin() + property.position + offset, this->data->begin() + property.position + offset + compressed_length);
            std::vector<char> output_data(array_length * sizeof(double));
            
            z_stream infstream;
            infstream.zalloc = Z_NULL;
            infstream.zfree = Z_NULL;
            infstream.opaque = Z_NULL;

            infstream.avail_in = compressed_length;
            infstream.next_in = reinterpret_cast<unsigned char*>(data.data());
            infstream.avail_out = static_cast<uint32_t>(output_data.size());
            infstream.next_out = reinterpret_cast<unsigned char*>(output_data.data());

            inflateInit(&infstream);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);

            data.clear();
            data.shrink_to_fit();
            data = output_data;
        }else{
            data = std::vector<char>(this->data->begin() + property.position + offset, this->data->begin() + property.position + offset + array_length * sizeof(double));
        }

        std::vector<double> output;
        for(uint32_t i=0; i<array_length; i++) {
            double value = *reinterpret_cast<const double*>(data.data() + i * sizeof(double));
            output.push_back(value);
        }

        return output;
    }

    std::vector<int64_t> FbxParser::ParseInt64Array(FBX_PROPERTY const& property)
    {
        std::vector<double> double_array = this->ParseFloat64Array(property);
        std::vector<int64_t> return_array(double_array.size());
        std::memcpy(return_array.data(), double_array.data(), double_array.size() * sizeof(int64_t));
        return return_array;
    }

    /**
    * Lecture d'un buffer de int32_t
    */
    std::vector<int32_t> FbxParser::ParseInt32Array(FBX_PROPERTY const& property)
    {
        size_t offset = sizeof(char);

        uint32_t array_length = DataPacker::Packer::ParseUint32(*this->data, property.position + offset);
        offset += sizeof(uint32_t);

        uint32_t encoding = DataPacker::Packer::ParseUint32(*this->data, property.position + offset);
        offset += sizeof(uint32_t);

        uint32_t compressed_length = DataPacker::Packer::ParseUint32(*this->data, property.position + offset);
        offset += sizeof(uint32_t);

        std::vector<char> data;

        if(encoding == ZLIB_COMPRESSION) {
            std::vector<char> output_data(array_length * sizeof(uint32_t));
            data = std::vector<char>(this->data->begin() + property.position + offset, this->data->begin() + property.position + offset + compressed_length);
            z_stream infstream;
            infstream.zalloc = Z_NULL;
            infstream.zfree = Z_NULL;
            infstream.opaque = Z_NULL;

            infstream.avail_in = compressed_length;
            infstream.next_in = reinterpret_cast<unsigned char*>(data.data());
            infstream.avail_out = static_cast<uint32_t>(output_data.size());
            infstream.next_out = reinterpret_cast<unsigned char*>(output_data.data());

            inflateInit(&infstream);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);
            data = output_data;
        }else{
            data = std::vector<char>(this->data->begin() + property.position + offset, this->data->begin() + property.position + offset + array_length * sizeof(uint32_t));
        }

        std::vector<int32_t> output;
        for(uint32_t i=0; i<array_length; i++) {
            int32_t value = *reinterpret_cast<int32_t*>(data.data() + i * sizeof(int32_t));
            output.push_back(value);
        }

        return output;
    }

    std::vector<float> FbxParser::ParseFloat32Array(FBX_PROPERTY const& property)
    {
        std::vector<int32_t> int_array = this->ParseInt32Array(property);
        std::vector<float> return_array(int_array.size());
        std::memcpy(return_array.data(), int_array.data(), int_array.size() * sizeof(float));
        return return_array;
    }

    /**
     * Alias local de la fonction ParseString de DataPacker::Packer
     */
    std::string FbxParser::ParseString(FBX_PROPERTY const& property)
    {
        return DataPacker::Packer::ParseString(*this->data, static_cast<uint32_t>(property.position+sizeof(uint32_t)+1), static_cast<uint32_t>(property.size-sizeof(uint32_t)-1));
    }
}