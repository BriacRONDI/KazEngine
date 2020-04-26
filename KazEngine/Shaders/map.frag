#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (set=0, binding=0) uniform sampler2D inTexture;

layout (set=1, binding=1) uniform MapProperties
{
	vec3 selection_position;
	uint display_selection;
	vec3 destination_position;
	uint display_destination;
} map;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

#define selection_radius 0.7
#define selection_border 0.1
#define selection_color vec4(0.0, 0.0, 1.0, 1.0)

#define destination_radius 0.17
#define destination_border 0.1
#define destination_color vec4(1.0, 0.0, 0.0, 1.0)

void main()
{
	outColor = texture(inTexture, inUV);
	
	if(map.display_selection > 0) {
		float dist = distance(map.selection_position, inPosition);
		float t = 1.0 + smoothstep(selection_radius, selection_radius + selection_border, dist) - smoothstep(selection_radius - selection_border, selection_radius, dist);
		outColor = mix(selection_color, outColor, t);
	}
	
	if(map.display_destination > 0 && distance(map.selection_position, map.destination_position) > selection_radius + destination_radius) {
		float dist = distance(map.destination_position, inPosition);
		float t = smoothstep(destination_radius, destination_radius + destination_border, dist);
		outColor = mix(destination_color, outColor, t);
	}	
}