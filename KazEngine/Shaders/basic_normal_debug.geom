#version 450

#define MAX_BONES				100
#define MAX_MESH_OFFSETS		100
#define MAX_BONE_PER_VERTEX		4
#define UINT32_MAX				4294967295

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (set=0, binding=0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
} uboCamera;

layout (set=0, binding=1) uniform InstanceUBO
{
	mat4 model;
	vec3 color;
} uboInstance;

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
layout (location = 4) in vec3 inPos[];

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
	float normalLength = 1.0f;
	
	for(int i=0; i<gl_in.length(); i++)
	{
		vec3 pos = gl_in[i].gl_Position.xyz;
		vec3 normal = inNormal[i].xyz;

		gl_Position = uboCamera.projection * uboCamera.view * (uboInstance.model * vec4(pos, 1.0));
		outColor = vec3(1.0, 0.0, 0.0);
		EmitVertex();

		gl_Position = uboCamera.projection * uboCamera.view * (uboInstance.model * vec4(pos + normal * normalLength, 1.0));
		outColor = vec3(0.0, 0.0, 1.0);
		EmitVertex();

		EndPrimitive();
	}
}
