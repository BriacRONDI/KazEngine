#version 450

layout (set=1, binding=0) uniform sampler2D inTexture;

layout (set=2, binding=0) readonly buffer IDs
{
	uint selection_count;
	uint entity_id[];
};

layout (set=3, binding=0) readonly buffer Entity
{
	mat4 model[];
};

struct MOVEMENT_DATA {
	vec2 destination;
	int moving;
	float radius;
};

layout (set=4, binding=0) readonly buffer MovementData
{
	MOVEMENT_DATA movement[];
};

layout (set=4, binding=1) readonly uniform MovementGroupCount
{
	uint movement_group_count;
};

struct MOVEMENT_GROUP {
	vec2 destination;
	int scale;
	float unit_radius;
	uint unit_count;
	uint fill_count;
	uint inside_count;
	uint padding;
};

layout (set=4, binding=2) readonly buffer MovementGroups
{
	MOVEMENT_GROUP group[];
};

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

// #define selection_diameter 0.45
#define selection_radius 0.5
#define selection_border 0.06
#define selection_color vec4(0.0, 0.0, 1.0, 1.0)

#define destination_radius 0.17
#define destination_border 0.1
#define destination_color vec4(1.0, 0.0, 0.0, 1.0)

void main()
{
	outColor = texture(inTexture, inUV);

	if(selection_count > 0) {
		for(int i=0; i<selection_count; i++) {
			vec3 entity_positon = vec3(model[entity_id[i]][3]);
			float dist = distance(entity_positon, inPosition);
			if(dist <= selection_radius + selection_border) {
				float t = 1.0 + smoothstep(selection_radius, selection_radius + selection_border, dist) - smoothstep(selection_radius - selection_border, selection_radius, dist);
				outColor = mix(selection_color, outColor, t);
			}
		}
	}
	
	if(movement_group_count > 0) {
		for(int i=0; i<movement_group_count; i++) {
			if(group[i].unit_count > 0) {
				float group_radius = (2 * group[i].scale - 1) * group[i].unit_radius;
				float dist = distance(group[i].destination, inPosition.xz);
				if(dist <= group_radius + selection_border) {
					float t = 1.0 + smoothstep(group_radius, group_radius + selection_border, dist) - smoothstep(group_radius - selection_border, group_radius, dist);
					outColor = mix(vec4(1.0, 0.0, 0.0, 1.0), outColor, t);
				}
			}
		}
	}
}