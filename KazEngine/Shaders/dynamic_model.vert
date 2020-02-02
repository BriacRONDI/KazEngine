#version 450

#define MAX_BONES				100
#define MAX_MESH_OFFSETS		100
#define MAX_BONE_PER_VERTEX		4
#define UINT32_MAX				4294967295

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec3  inNormal;
layout (location = 2) in vec2  inUV;
layout (location = 3) in vec4  inBoneWeights;
layout (location = 4) in ivec4 inBoneIDs;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=0, binding=1) uniform Entity
{
	mat4 model;
	// uint frame_id;
} entity;

layout (set=2, binding=0) uniform Skeleton
{
	mat4 bones[MAX_BONES];
	uint bones_per_frame;
} skeleton;

layout (set=2, binding=1) uniform Mesh
{
	mat4 offsets[MAX_MESH_OFFSETS];
} mesh;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec4 outBoneWeights;
layout (location = 5) out ivec4 outBoneIDs;
layout (location = 6) out vec3 outPos;

/*out gl_PerVertex 
{
	vec4 gl_Position;   
};*/

vec3 MatrixMultT(mat4 matrix, vec3 vertex)
{
	return vec3(
		matrix[0][0] * vertex[0] + matrix[1][0] * vertex[1] + matrix[2][0] * vertex[2] + matrix[3][0],
		matrix[0][1] * vertex[0] + matrix[1][1] * vertex[1] + matrix[2][1] * vertex[2] + matrix[3][1],
		matrix[0][2] * vertex[0] + matrix[1][2] * vertex[1] + matrix[2][2] * vertex[2] + matrix[3][2]
	);
}

void main() 
{
	mat4 boneTransform = mat4(0);
	mat4 modelView = camera.view * entity.model;
	bool has_bone = false;
	float total_weight;

	outUV = inUV;
	outNormal = mat3(modelView) * inNormal;
	outBoneWeights = inBoneWeights;
	outBoneIDs = inBoneIDs;
	outPos = inPos;

	/*if(entity.bone_id != UINT32_MAX) {
	
		has_bone = true;
		total_weight = 1.0f;
		boneTransform = skeleton.bones[entity.bone_id];
		
	}else{*/
	
		total_weight = 0.0f;
		for(int i=0; i<MAX_BONE_PER_VERTEX; i++) {
			if(inBoneWeights[i] == 0) break;
			boneTransform += skeleton.bones[inBoneIDs[i]] * mesh.offsets[inBoneIDs[i]] * inBoneWeights[i];
			total_weight += inBoneWeights[i];
			has_bone = true;
		}
		
	//}
	
	if(!has_bone) {
	
		vec4 pos = modelView * vec4(inPos, 1.0);
		outLightVec = vec3(1.5f, 1.5f, 1.5f) - pos.xyz;
		outViewVec = -pos.xyz;
	
		gl_Position = camera.projection * modelView * vec4(inPos, 1.0);
		
	}else{
	
		vec3 transformed_vertex = MatrixMultT(boneTransform, inPos) / total_weight;
		
		vec4 pos = modelView * vec4(transformed_vertex, 1.0);
		outLightVec = vec3(1.5f, 1.5f, 1.5f) - pos.xyz;
		outViewVec = -pos.xyz;
		
		gl_Position = camera.projection * pos;
		
	}
}