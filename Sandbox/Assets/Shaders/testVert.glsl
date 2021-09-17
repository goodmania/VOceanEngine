#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push 
{
  mat4 transform; // projection * view * model
  mat4 normalMatrix;
} push;

layout(std430, binding = 0) buffer H0In
{
	Ocean OceanIn[];
};

layout(std430, binding = 1) buffer H0Out
{
	Ocean OceanOut[];
};

layout(binding = 2) uniform UBO
{
	float deltaT;
	uint meshSize;
	uint OceanSizeLx;
	uint OceanSizeLz;
} ubo;

struct Ocean
{
	vec2 h;
	vec4 Pos;
	vec4 UV;
	vec4 Normal;
};

vec4 ui_calcPos(uint ui_idx)
{
	uint x = ui_idx % N;
	uint z = ui_idx / N;
	vec2 h = d_ht[ui_idx];
	return float4((1.0 * x - halfN) * dx, h.x, (1.0 * z - halfN) * dz, 1);
}

void main()
{
	gl_Position = push.transform * vec4(pos, 1.0);
	fragColor = color;
}