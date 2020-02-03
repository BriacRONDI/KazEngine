#version 450

layout (location = 0) in vec3 inColor;
// layout (location = 1) in vec3 inFragPos;

layout (set=0, binding=0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec3 position;
} camera;

/*layout (set=0, binding=2) uniform Lighting
{
	vec3 color;
	vec3 position;
	float ambient_strength;
	float specular_strength;
}light;*/

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inColor, 1.0);
	/*vec3 normal = normalize(inNormal);
	
	// Ambient light
	vec3 ambient_color = inColor * light.color * light.ambient_strength;
	
	// Diffuse light
	vec3 light_direction = normalize(light.position - inFragPos);
	float diff = max(dot(normal, light_direction), 0.0);
	vec3 diffuse_color = diff * light.color;
	
	// Specular light
	// vec3 view_direction = normalize(inFragPos - view_position);
	vec3 view_direction = normalize(inFragPos - camera.position);
	vec3 reflect_direction = reflect(-light_direction, normal);  
	float specular = pow(max(dot(view_direction, reflect_direction), 0.0), 8); // Shininess
	vec3 specular_color = light.specular_strength * specular * light.color;  
	
	vec3 color = (ambient_color + diffuse_color + specular_color) * inColor;
    outColor = vec4(color, 1.0);*/
}