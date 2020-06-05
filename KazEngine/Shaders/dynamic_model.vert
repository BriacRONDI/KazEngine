#version 450

#define MAX_BONE_PER_VERTEX		4

layout (location = 0)  in vec3  inPos;
layout (location = 1)  in vec2  inUV;
layout (location = 2)  in vec4  inBoneWeights;
layout (location = 3)  in ivec4 inBoneIDs;

layout (location = 4)  in mat4 model;
layout (location = 8)  in uint selected;

layout (location = 9)  in uint animation_id;
layout (location = 10)  in uint frame_id;
// layout (location = 11) in vec2 padding;


layout (set=1, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;


/*layout (set=2, binding=0) buffer ID
{
	uint entity_id[];
};

struct Properties {
	mat4 model;
	uint animation_id;
	uint frame_id;
	// ivec2 padding;
};

layout (set=2, binding=1) buffer Entity
{
	Properties entity[];
};*/

layout (set=2, binding=0) buffer Skeleton
{
	mat4 bones[];
} skeleton;

layout (set=2, binding=1) buffer OffsetIDs
{
	uint offset_ids[];
};

layout (set=2, binding=2) buffer Offsets
{
	mat4 offsets[];
};

layout (set=2, binding=3) buffer Animations
{
	uint bone_count;
	uint first_frame_id[];
}animations;

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
	// Properties entity_props = entity[entity_id[gl_InstanceIndex]];
	mat4 modelView = camera.view * model;
	// mat4 modelView = camera.view * entity_props.model;
	bool has_bone = false;
	float total_weight = 0.0f;

	outUV = inUV;

	for(int i=0; i<MAX_BONE_PER_VERTEX; i++) {
		if(inBoneWeights[i] == 0) break;
		
		// uint frame_id = animations.first_frame_id[entity_props.animation_id];
		// boneTransform += skeleton.bones[animations.bone_count * entity_props.frame_id + inBoneIDs[i]] * offsets[offset_ids[inBoneIDs[i]]] * inBoneWeights[i];
		// boneTransform += skeleton.bones[animations.bone_count * frame_id + inBoneIDs[i]] * offsets[offset_ids[inBoneIDs[i]]] * inBoneWeights[i];
		boneTransform += skeleton.bones[0] * offsets[offset_ids[inBoneIDs[i]]] * inBoneWeights[i];
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