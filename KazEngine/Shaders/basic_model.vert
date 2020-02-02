#version 450

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec3  inNormal;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec3 position;
} camera;

layout (set=0, binding=1) uniform Entity
{
	mat4 model;
	vec3 color;
} entity;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outFragPos;

void main() 
{
	mat4 modelView = camera.view * entity.model;
	
	outNormal = mat3(entity.model) * inNormal;
	outColor = entity.color;
    outFragPos = (entity.model * vec4(inPos, 1.0)).xyz;
	
	gl_Position = camera.projection * modelView * vec4(inPos, 1.0);
}