#version 450

layout (location = 0) in vec3  inPos;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec3 position;
} camera;

layout (set=1, binding=0) uniform Entity
{
	mat4 model;
	vec3 color;
} entity;

layout (location = 0) out vec3 outColor;

void main() 
{
	outColor = entity.color;
	gl_Position = camera.projection * camera.view * entity.model * vec4(inPos, 1.0);
}