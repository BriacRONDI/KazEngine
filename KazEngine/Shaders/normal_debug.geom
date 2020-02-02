#version 450

#define MAX_BONES				100
#define MAX_MESH_OFFSETS		100
#define MAX_BONE_PER_VERTEX		4
#define UINT32_MAX				4294967295

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=0, binding=1) uniform Entity
{
	mat4 model;
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

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];
layout (location = 2) in vec3 inViewVec[];
layout (location = 3) in vec3 inLightVec[];
layout (location = 4) in vec4 inBoneWeights[];
layout (location = 5) in ivec4 inBoneIDs[];
layout (location = 6) in vec3 inPos[];

layout (location = 0) out vec3 outColor;

vec3 MatrixMultT(mat4 matrix, vec3 vertex)
{
	return vec3(
		matrix[0][0] * vertex[0] + matrix[1][0] * vertex[1] + matrix[2][0] * vertex[2] + matrix[3][0],
		matrix[0][1] * vertex[0] + matrix[1][1] * vertex[1] + matrix[2][1] * vertex[2] + matrix[3][1],
		matrix[0][2] * vertex[0] + matrix[1][2] * vertex[1] + matrix[2][2] * vertex[2] + matrix[3][2]
	);
}

void main(void)
{	
	float normalLength = 100.0f;
	for(int i=0; i<gl_in.length(); i++)
	{
		// vec3 pos = gl_in[i].gl_Position.xyz;
		// vec3 normal = inNormal[i].xyz;
		
		mat4 boneTransform = mat4(0);
		bool has_bone = false;
		float total_weight;
		
		total_weight = 0.0f;
		for(int j=0; j<MAX_BONE_PER_VERTEX; j++) {
			if(inBoneWeights[i][j] == 0) break;
			boneTransform += skeleton.bones[inBoneIDs[i][j]] * mesh.offsets[inBoneIDs[i][j]] * inBoneWeights[i][j];
			total_weight += inBoneWeights[i][j];
			has_bone = true;
		}
		
		if(!has_bone) {
	
			gl_Position = camera.projection * camera.view * (entity.model * vec4(inPos[i], 1.0));
			outColor = vec3(1.0, 0.0, 0.0);
			EmitVertex();

			gl_Position = camera.projection * camera.view * (entity.model * vec4(inPos[i] + inNormal[i] * normalLength, 1.0));
			outColor = vec3(0.0, 0.0, 1.0);
			EmitVertex();
			
			EndPrimitive();
			
		}else{
		
			vec3 transformed_vertex = MatrixMultT(boneTransform, inPos[i]) / total_weight;
			gl_Position = camera.projection * camera.view * (entity.model * vec4(transformed_vertex, 1.0));
			outColor = vec3(1.0, 0.0, 0.0);
			EmitVertex();
			
			gl_Position = camera.projection * camera.view * (entity.model * vec4(transformed_vertex + inNormal[i] * normalLength, 1.0));
			outColor = vec3(0.0, 0.0, 1.0);
			EmitVertex();
			
			EndPrimitive();
		}
	}
}
