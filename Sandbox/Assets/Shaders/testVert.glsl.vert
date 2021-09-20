#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push 
{
  mat4 transform; // projection * view * model
  mat4 normalMatrix;
} push;

layout(std430, binding = 0) buffer Ht_dmyBuffer
{
	vec2 Ht_dmyBuffers[];
};

layout(std140, binding = 1) uniform UBO
{
	float deltaT;
	uint meshSize;
	uint OceanSizeLx;
	uint OceanSizeLz;
} ubo;

vec4 ui_calcPos(uint ui_idx)
{
	uint N = ubo.meshSize;
	float halfN = ubo.meshSize * 0.5f;
	float dx = 1.0f * ubo.OceanSizeLx / ubo.meshSize;
	float dz  =1.0f * ubo.OceanSizeLz / ubo.meshSize;

	float x = ui_idx % N;
	float z = ui_idx / N;

	vec2 h = Ht_dmyBuffers[ui_idx];

	return vec4((1.0 * x - halfN) * dx, h.x, (1.0 * z - halfN) * dz, 1);
}

void main()
{
	// gl_Position = push.transform * ui_calcPos(gl_VertexIndex);
	gl_Position = push.transform * vec4(pos.x, Ht_dmyBuffers[gl_VertexIndex].x, pos.z, 1.0);
	fragColor = color;
}