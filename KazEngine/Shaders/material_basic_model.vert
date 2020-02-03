#version 450

layout (location = 0) in vec3  inPos;

layout (set=0, binding=0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
} camera;

layout (set=0, binding=1) uniform MeshUBO
{
	mat4 model;
} mesh;


void main() 
{
	mat4 modelView = camera.view * mesh.model;
	gl_Position = camera.projection * modelView * vec4(inPos, 1.0);
}