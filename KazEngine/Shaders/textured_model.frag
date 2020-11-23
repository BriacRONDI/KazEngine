#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (set=0, binding=0) uniform sampler2D inTexture[2];

layout (location = 0) in vec2 inUV;


layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(inTexture[0], inUV);
}