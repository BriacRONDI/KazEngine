#version 450

#define MAX_BONES			100
#define MAX_BONE_PER_VERTEX 8
#define UINT32_MAX 			4294967295

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (set=0, binding=1) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	uint bone_id;
	mat4 bones[MAX_BONES];
} ubo;

layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outColor;

void main(void)
{	
	float normalLength = 2.0f;
	for(int i=0; i<gl_in.length(); i++)
	{
		vec3 pos = gl_in[i].gl_Position.xyz;
		vec3 normal = inNormal[i].xyz;

		gl_Position = ubo.projection * ubo.view * (ubo.model * vec4(pos, 1.0));
		outColor = vec3(1.0, 0.0, 0.0);
		EmitVertex();

		gl_Position = ubo.projection * ubo.view * (ubo.model * vec4(pos + normal * normalLength, 1.0));
		outColor = vec3(0.0, 0.0, 1.0);
		EmitVertex();

		EndPrimitive();
	}
}
