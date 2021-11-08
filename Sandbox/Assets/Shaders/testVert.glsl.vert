#version 450

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 color;
layout(location = 0) out vec4 fragColor;

struct Ocean
{
	vec2 H_y;
	vec2 H_x;
	vec2 H_z;
	vec2 Dx;
	vec2 Dz;
};

layout(push_constant) uniform Push 
{
  mat4 transform; // projection * view * model
  mat4 normalMatrix;
} push;

layout(std430, binding = 0) buffer HtBuffer
{
	Ocean HtBuffers[];
};

layout(std140, binding = 1) uniform UBO
{
	float deltaT;
	highp uint meshSize;
	highp uint OceanSizeLx;
	highp uint OceanSizeLz;
} ubo;

void main()
{
	float lamda = -2.0f;
	gl_Position = push.transform * vec4(
		pos.x + HtBuffers[gl_VertexIndex].Dx.y * lamda,
		HtBuffers[gl_VertexIndex].H_y.x,
		pos.z + HtBuffers[gl_VertexIndex].Dz.y * lamda,
		1.0);

	fragColor = color;
}