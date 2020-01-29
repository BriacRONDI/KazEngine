#version 450

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

layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outColor;

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
