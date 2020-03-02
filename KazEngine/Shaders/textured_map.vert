#version 450

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec2  inUV;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = inUV;
	gl_Position = camera.projection * camera.view * vec4(inPos, 1.0);
}