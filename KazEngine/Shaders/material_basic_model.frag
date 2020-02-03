#version 450

layout(push_constant) uniform Material 
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float opacity;
} material;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = material.diffuse;
}