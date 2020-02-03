#version 450

layout (set=1, binding=0) uniform sampler2D inTexture;

layout (location = 0) in vec2 inUV;

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float opacity;
};

layout(push_constant) uniform inMesh
{
	layout(offset = 0) Material material;
};

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(inTexture, inUV);
}