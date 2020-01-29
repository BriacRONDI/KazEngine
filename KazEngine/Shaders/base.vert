#version 450

#define MAX_BONES			100
#define MAX_BONE_PER_VERTEX 8
#define UINT32_MAX 			4294967295

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec3  inNormal;
layout (location = 2) in vec2  inUV;
layout (location = 3) in vec4  inBoneWeights;
layout (location = 4) in ivec4 inBoneIDs;

layout (set=0, binding=1) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	uint bone_id;
	mat4 bones[MAX_BONES];
} ubo;


layout (location = 0) out vec3 outNormal;

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
	mat4 modelView = ubo.view * ubo.model;
	bool has_bone = false;
	float total_weight;
	
	outNormal = inNormal;
	
	if(ubo.bone_id != UINT32_MAX) {
	
		has_bone = true;
		total_weight = 1.0f;
		boneTransform = ubo.bones[ubo.bone_id];
		
	}else{
	
		total_weight = 0.0f;
		for(int i=0; i<MAX_BONE_PER_VERTEX; i++) {
			if(inBoneWeights[i] == 0) break;
			boneTransform += ubo.bones[inBoneIDs[i]] * inBoneWeights[i];
			total_weight += inBoneWeights[i];
			has_bone = true;
		}
		
	}
	
	if(!has_bone) {
	
		gl_Position = vec4(inPos, 1.0);
		
	}else{
	
		gl_Position = vec4(MatrixMultT(boneTransform, inPos) / total_weight, 1.0);
		
	}
}