#version 450

#define MAX_BONES				100
#define MAX_BONE_OFFSETS		100
#define MAX_BONE_PER_VERTEX		4

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec2  inUV;

layout (push_constant) uniform BoneID
{
	layout(offset = 52) uint bone_id;
};

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=1, binding=0) uniform Entity
{
	mat4 model;
	uint frame_id;
} entity;

layout (set=3, binding=0) uniform Skeleton
{
	uint bones_per_frame;
} meta;

layout (set=3, binding=1) buffer Skeleton
{
	mat4 bones[];
} skeleton;

layout (set=3, binding=2) uniform BoneOffsets
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
	outUV = inUV;
	
	mat4 boneTransform = skeleton.bones[meta.bones_per_frame * entity.frame_id + bone_id] * offsets[offset_ids[bone_id]];
	vec3 transformed_vertex = MatrixMultT(boneTransform, inPos);
	gl_Position = camera.projection * camera.view * entity.model * vec4(transformed_vertex, 1.0);
}