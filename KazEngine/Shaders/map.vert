#version 450

layout (location = 0) in vec3  inPos;
layout (location = 1) in vec2  inUV;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
} camera;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outPosition;
// layout (location = 2) flat out vec3 outSelectionCenter;

void main() 
{
	outUV = inUV;
	mat4 MVP = camera.projection * camera.view;
	vec4 position = MVP * vec4(inPos, 1.0);
	gl_Position = position;
	outPosition = position.xyz;
	// outSelectionCenter = (MVP * vec4(map.selection, 1.0)).xyz;
}