#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inViewVec;
layout (location = 2) in vec3 inLightVec;

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
	/*vec3 N = inNormal;
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.0) * material.diffuse.rgb;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * material.specular.rgb;
	outColor = vec4((material.ambient.rgb + diffuse) + specular, 1.0 - material.opacity);*/
	outColor = material.diffuse;
}