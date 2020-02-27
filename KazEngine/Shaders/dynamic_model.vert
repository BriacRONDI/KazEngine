#version 450

#define MAX_BONES				100
#define MAX_BONE_OFFSETS		100
#define MAX_BONE_PER_VERTEX		4

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec2  inUV;
layout (location = 2) in vec4  inBoneWeights;
layout (location = 3) in ivec4 inBoneIDs;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=0, binding=1) uniform Entity
{
	mat4 model;
	uint frame_id;
	uint animation_id;
} entity;

layout (set=2, binding=0) uniform Skeleton
{
	uint bones_per_frame;
} meta;

layout (set=2, binding=1) buffer Skeleton
{
	mat4 bones[];
} skeleton;

layout (set=2, binding=2) uniform BoneOffsets
{
	uint offset_ids[MAX_BONES];
	mat4 offsets[MAX_BONE_OFFSETS];
};

layout (location = 0) out vec2 outUV;

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
	float total_weight = 0.0f;

	outUV = inUV;

	for(int i=0; i<MAX_BONE_PER_VERTEX; i++) {
		if(inBoneWeights[i] == 0) break;
		boneTransform += skeleton.bones[entity.animation_id + meta.bones_per_frame * entity.frame_id + inBoneIDs[i]] * offsets[offset_ids[inBoneIDs[i]]] * inBoneWeights[i];
		total_weight += inBoneWeights[i];
		has_bone = true;
	}
		
	if(!has_bone) {
	
		gl_Position = camera.projection * modelView * vec4(inPos, 1.0);
		
	}else{
	
		vec3 transformed_vertex = MatrixMultT(boneTransform, inPos) / total_weight;
		gl_Position = camera.projection * modelView * vec4(transformed_vertex, 1.0);
		
	}
}