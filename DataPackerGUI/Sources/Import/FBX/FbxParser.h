#pragma once

#define ZLIB_COMPRESSION 1
#define NO_ENCODING 0
#define FBX_TIME_RATIO 46186158000

#define FBX_PROPERTY_DATA(node,property) node.at(property).at(0).properties.at(0)

#include <zlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <chrono>
#include <set>

#include "../Parser/Parser.h"
#include "../../EngineModel/Sources/Model.h"
#include "../../DataPacker/Sources/DataPacker.h"
#include "../../Maths/Sources/Maths.h"
#include "../../Tools/Sources/Tools.h"
#include "../../LogManager/Sources/LogManager.h"

namespace DataPackerGUI
{
    typedef std::chrono::duration<int64_t, std::ratio<1,FBX_TIME_RATIO>> fbx_duration;

    class FbxParser : public Parser
    {
        public :
        
            virtual std::vector<char> ParseData(std::vector<char> const& data, std::string const& texture_directory = {}, std::string const& package_directory = "/");

        private :

            //////////////////////////////
            //// Structures décrivant ////
            ////   le format FBX      ////
            //////////////////////////////

            struct FBX_PROPERTY {
                char type;
                size_t position;
                size_t size;
            };

            struct FBX_PROPERTY70 {
                std::string name;
                std::string type1;
                std::string type2;
                std::string flag;
                std::vector<FBX_PROPERTY> value;
            };

            struct FBX_NODE {
                int64_t id;
                std::string name;
                std::string attribute_name;
                std::string attribute_type;
                std::vector<FBX_PROPERTY> properties;
                std::map<std::string, std::vector<FBX_NODE>> children;
                
                FBX_NODE() : id(-1) {}
            };

            struct FBX_TEXTURE {
                int64_t id;
                std::string name;
                std::string RelativeFilename;
            };

            struct FBX_MATERIAL {
                int64_t id;
                uint8_t unique_id;
                std::string name;
                std::string ShadingModel;
                std::map<std::string, FBX_TEXTURE*> textures;

                std::array<double, 3> EmissiveColor;
                double EmissiveFactor;
                std::array<double, 3> AmbientColor;
                double AmbientFactor;
                std::array<double, 3> DiffuseColor;
                double DiffuseFactor;
                std::array<double, 3> SpecularColor;
                double SpecularFactor;
                std::array<double, 3> Bump;
                double BumpFactor;
                std::array<double, 3> TransparentColor;
                double TransparencyFactor;
                double Shininess;

                FBX_MATERIAL() :
                    id(-1),
                    EmissiveColor({}), EmissiveFactor(0),
                    AmbientColor({}), AmbientFactor(0),
                    DiffuseColor({}), DiffuseFactor(0),
                    Bump({}), BumpFactor(0),
                    TransparentColor({}), TransparencyFactor(0),
                    Shininess(0) {
                }
            };

            struct FBX_LAYER {
                std::string Type;
                uint32_t TypedIndex;
            };

            struct FBX_LAYER_ELEMENT {
                std::string mapping_information_type;
                std::string reference_information_type;
                std::vector<int32_t> indices;
                std::vector<double> buffer;
            };

            struct FBX_BONE {
                int64_t id;
                int64_t bone_id;
                std::string name;
                Maths::Matrix4x4 transform_link;
                Maths::Matrix4x4 transform;
                Maths::Matrix4x4 deformation;
                uint32_t link_mode;
                std::vector<double> weights;
                std::vector<int32_t> indices;

                FBX_BONE() : id(-1), bone_id(-1), link_mode(0) {}
            };

            struct FBX_SKELETON {
                int64_t id;
                std::string name;
                std::string mesh_name;
                std::vector<FBX_BONE> bones;

                FBX_SKELETON() : id(-1) {}
            };

            struct FBX_MESH_GEOMETRY {
                int64_t id;
                std::string name;
                std::vector<double> vertices;
                std::vector<int32_t> vertex_indices;
                FBX_LAYER_ELEMENT uv;
                FBX_LAYER_ELEMENT color;
                FBX_LAYER_ELEMENT normal;
                FBX_LAYER_ELEMENT material;
                std::vector<FBX_LAYER> layers;
                FBX_NODE model;
                Maths::Matrix4x4 global_transformation;
                Maths::Matrix4x4 local_transformation;
                FBX_BONE cluster;
                FBX_SKELETON skeleton;
            };

            struct FBX_ANIMATION_CURVE {
                int64_t id;
                std::vector<fbx_duration> key_time;
                std::vector<float> key_value;

                FBX_ANIMATION_CURVE() : id(-1), key_time({}), key_value({}) {}
            };

            struct FBX_CURVE_NODE {
                int64_t id;
                std::string attribute_name;
                Maths::Vector3 vector;
                double morph;
                std::map<std::string, FBX_ANIMATION_CURVE> curves;

                FBX_CURVE_NODE() : vector(0,0,0), morph(0), id(-1) {}
            };

            struct FBX_ANIMATION_NODE {
                int64_t id;
                FBX_NODE model;
                std::string morph_name;
                Maths::Matrix4x4 transform;
                std::map<std::string, FBX_CURVE_NODE> curve_nodes;

                FBX_ANIMATION_NODE() : id(-1) {}
            };

            struct FBX_ANIMATION_LAYER {
                int64_t id;
                std::string name;
                std::map<std::string, FBX_ANIMATION_NODE> animation_nodes;
                std::chrono::milliseconds duration;
            };

            /////////////////////////////////
            // Relations entre les objects //
            /////////////////////////////////

            struct FBX_RELATION {
                int64_t id;
                std::string relationship;
            };

            struct FBX_CONNECTION {
                std::vector<FBX_RELATION> parents;
                std::vector<FBX_RELATION> children;
            };

            struct CONNECTION_TREE {
                int64_t id;
                std::vector<CONNECTION_TREE> children;

                CONNECTION_TREE() : CONNECTION_TREE(-1) {}
                CONNECTION_TREE(int64_t id) : id(id) {}
            };

            /////////////
            // Membres //
            /////////////

            uint32_t version;
            uint8_t next_material_id;
            uint32_t next_referenced_bone_id;

            std::map<int64_t, FBX_CONNECTION> connections;
            std::vector<char> const* data;
            std::vector<FBX_TEXTURE> textures;
            std::vector<FBX_MATERIAL> materials;
            std::vector<FBX_MESH_GEOMETRY> mesh_geometries;
            std::vector<Model::Mesh> meshes;
            std::map<std::string, Model::Mesh::MATERIAL> engine_materials;
            std::vector<FBX_SKELETON> skeletons;
            std::vector<FBX_CURVE_NODE> curve_nodes;
            std::vector<FBX_ANIMATION_LAYER> animation_layers;
            std::vector<CONNECTION_TREE> connection_tree;
            std::map<int64_t, uint32_t> referenced_bones;
            std::map<std::string, Model::Bone> bone_trees;
            std::map<int64_t, uint32_t> used_bones;
            std::map<int64_t, uint32_t> unused_bones;
            uint32_t next_used_bone_id;
            Model::Bone null_bone;

            //////////////
            // Méthodes //
            //////////////

            bool isBinaryFbx();
            inline uint32_t GetVersion();
            FBX_NODE ParseNode(size_t& position);
            bool EndOfContent(size_t position);
            FBX_PROPERTY ParseRawProperty(size_t position);
            bool IsNullNode(FBX_NODE const& node);
            void ParseConnections(std::vector<FBX_NODE> const& nodes);
            FBX_NODE const GetMainNode(std::string const& node_name, std::vector<FBX_NODE> const& nodes);
            std::vector<FbxParser::FBX_NODE> const GetObjectNode(std::string const& object_name, std::vector<FBX_NODE> const& nodes);
            void ParseTextures(std::vector<FBX_NODE> const& nodes);
            void ParseMaterials(std::vector<FBX_NODE> const& nodes);
            std::map<std::string, FBX_PROPERTY70> ParseProperties70(FBX_NODE const& node);
            std::array<double, 3> ParseColor(FBX_PROPERTY70 const& property70);
            inline std::array<double, 3> ParseVector(FBX_PROPERTY70 const& property70){ return this->ParseColor(property70); }
            void ParseMeshGeometry(std::vector<FBX_NODE> const& nodes);
            FBX_LAYER_ELEMENT ParseLayerElement(FBX_NODE const& layer_node, std::string const& buffer_name, std::string const& indices_name);
            std::vector<FBX_LAYER> ParseLayers(FBX_NODE const& layer_node);
            Maths::Matrix4x4 ComputeTransformation(FBX_NODE const& node, std::vector<FBX_NODE> const& models);
            Maths::Matrix4x4 ComputeTransformation(FBX_NODE const& node, Maths::Matrix4x4 const& parent_transformation);
            void ParseMeshes(std::vector<FBX_NODE> const& nodes);
            inline std::string ParseString(FBX_PROPERTY const& property);
            void ParseSkeletons(std::vector<FBX_NODE> const& nodes);
            void ParseAnimationCurveNode(std::vector<FBX_NODE> const& nodes);
            void ParseAnimationCurve(std::vector<FBX_NODE> const& nodes);
            void ParseAnimationLayer(std::vector<FBX_NODE> const& nodes);
            void ParseAnimationStack(std::vector<FBX_NODE> const& nodes);
            void ParseLimb(std::vector<FBX_NODE> const& nodes);
            std::array<std::vector<Model::KEYFRAME>, 3> ParseCurveNode(FBX_CURVE_NODE const& curve_node);
            void ComputeAnimations();
            FbxParser::FBX_NODE GetRootModel(FBX_NODE const& node, std::vector<FBX_NODE> const& models);
            Model::Bone GetBoneTree(FBX_NODE const& node, Model::Bone const& parent, std::vector<FBX_NODE> const& models);
            Model::Bone CreateBone(FBX_NODE const& node, Model::Bone const& parent);
            Model::Bone& FindBone(uint32_t bone_id, Model::Bone& parent);
            Model::Bone RebuildBoneTree(Model::Bone& bone, bool& skeep_me);
            FbxParser::FBX_BONE FindCluster(int64_t id);
            void ComputeClusterDeformation(FBX_MESH_GEOMETRY& mesh_geometry);
            Model::Bone GetBone(FBX_BONE const& node, Model::Bone const& bone_tree);
            Maths::Matrix4x4 GetBoneGlobalTransform(Model::Bone const& bone, Model::Bone const& bone_tree);
            Model::Bone GetParentBone(Model::Bone const& bone, Model::Bone const& bone_tree);
            void ApplyBonesOffsets(FBX_MESH_GEOMETRY& mesh_geometry, std::string const& mesh_name);
            void SetBoneOffset(uint32_t bone_id, std::string const& mesh_name, Maths::Matrix4x4 const& offset, Model::Bone& bone_tree);

            std::vector<float> ParseFloat32Array(FBX_PROPERTY const& property);
            std::vector<int64_t> ParseInt64Array(FBX_PROPERTY const& property);
            std::vector<double> ParseFloat64Array(FBX_PROPERTY const& property);
            std::vector<int32_t> ParseInt32Array(FBX_PROPERTY const& property);
    };
}