#version 450

layout (set=1, binding=0) uniform sampler2D inTexture;

struct Properties {
	mat4 model;
	uint selected;
};

layout (set=2, binding=0) readonly buffer IDs
{
	uint selection_count;
	uint entity_id[];
};

layout (set=2, binding=1) readonly buffer Entity
{
	Properties entity[];
};

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

#define selection_radius 0.45
#define selection_border 0.06
#define selection_color vec4(0.0, 0.0, 1.0, 1.0)

#define destination_radius 0.17
#define destination_border 0.1
#define destination_color vec4(1.0, 0.0, 0.0, 1.0)

void main()
{
	outColor = texture(inTexture, inUV);
	
	/*if(selection_count > 0) {
		for(int i=0; i<selection_count; i++) {
			float dist = distance(entity_positon[i], inPosition);
			if(dist <= selection_radius + selection_border) {
				float t = 1.0 + smoothstep(selection_radius, selection_radius + selection_border, dist) - smoothstep(selection_radius - selection_border, selection_radius, dist);
				outColor = mix(selection_color, outColor, t);
			}
		}
	}*/
	
	if(selection_count > 0) {
		for(int i=0; i<selection_count; i++) {
			vec3 entity_positon = vec3(entity[entity_id[i]].model[3]);
			float dist = distance(entity_positon, inPosition);
			if(dist <= selection_radius + selection_border) {
				float t = 1.0 + smoothstep(selection_radius, selection_radius + selection_border, dist) - smoothstep(selection_radius - selection_border, selection_radius, dist);
				outColor = mix(selection_color, outColor, t);
			}
		}
	}
	
	
	/*if(map.display_selection > 0) {
		float dist = distance(map.selection_position, inPosition);
		float t = 1.0 + smoothstep(selection_radius, selection_radius + selection_border, dist) - smoothstep(selection_radius - selection_border, selection_radius, dist);
		outColor = mix(selection_color, outColor, t);
	}
	
	if(map.display_destination > 0 && distance(map.selection_position, map.destination_position) > selection_radius + destination_radius) {
		float dist = distance(map.destination_position, inPosition);
		float t = smoothstep(destination_radius, destination_radius + destination_border, dist);
		outColor = mix(destination_color, outColor, t);
	}*/
}