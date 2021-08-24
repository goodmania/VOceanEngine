#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;


layout(push_constant) uniform Push 
{
  mat4 transform; // projection * view * model
  mat4 normalMatrix;
} push;

void main()
{
	gl_Position = push.transform * vec4(pos, 1.0);
	fragColor = color;
}