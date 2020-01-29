#version 450

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec3  inNormal;

layout (set=0, binding=0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=0, binding=1) uniform MeshUBO
{
	mat4 model;
} mesh;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outViewVec;
layout (location = 2) out vec3 outLightVec;

void main() 
{
	mat4 modelView = camera.view * mesh.model;
	vec4 pos = modelView * vec4(inPos, 1.0);
	
	outNormal = mat3(modelView) * inNormal;
    outViewVec = -pos.xyz;
    outLightVec = vec3(1.5f, 1.5f, 1.5f) - pos.xyz;
	
	gl_Position = camera.projection * modelView * vec4(inPos, 1.0);
}