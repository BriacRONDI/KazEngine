#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (set=0, binding=0) uniform sampler2D inTexture[1];

layout (location = 0) in vec2 inUV;

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float opacity;
	int texture_id;
};

layout(push_constant) uniform inMesh
{
	layout(offset = 0) Material material;
};

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(inTexture[material.texture_id], inUV);
	// outColor = texture(inTexture[0], inUV);
}