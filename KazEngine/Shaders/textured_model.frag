#version 450

layout (set=1, binding=0) uniform sampler2D inTexture;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float opacity;
};

layout(push_constant) uniform inMesh
{
	Material material;
};

layout (location = 0) out vec4 outColor;

void main()
{
	/*vec4 color = texture(inTexture, inUV);
	vec3 N = inNormal;
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.0) * material.diffuse.rgb;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * material.specular.rgb;
	outColor = vec4((material.ambient.rgb + diffuse) * color.rgb + specular, 1.0 - material.opacity);*/
	outColor = material.diffuse;
}