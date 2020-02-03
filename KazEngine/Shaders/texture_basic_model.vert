#version 450

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec3  inUV;

layout (set=0, binding=0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=0, binding=1) uniform MeshUBO
{
	mat4 model;
} mesh;

layout (location = 0) out vec3 outUV;

void main() 
{
	outUV = inUV;
	gl_Position = camera.projection * camera.view * mesh.model * vec4(inPos, 1.0);
}