#version 450

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec3  inNormal;

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

layout (location = 0) out vec3 outNormal;

void main() 
{
	outNormal = inNormal;
	gl_Position = vec4(inPos, 1.0);
}