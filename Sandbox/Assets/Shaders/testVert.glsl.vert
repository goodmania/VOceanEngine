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
  mat4 ModelMatrix; // projection * view * model
  mat4 normalMatrix;
} push;

layout(std430, binding = 0) buffer HtBuffer
{
	Ocean HtBuffers[];
};

layout(std140, binding = 1) uniform UBO
{
	float deltaT;
	float lamda;
	highp uint meshSize;
	highp uint OceanSizeLx;
	highp uint OceanSizeLz;
} ubo;

layout(std140, binding = 2) uniform GlobalUBO
{
	mat4 ProjectionView;
	vec3 lightDirection;
} globalUbo;

const float AMBIENT = 0.02f;

void main()
{
	gl_Position = globalUbo.ProjectionView * push.ModelMatrix * vec4(
		pos.x + (HtBuffers[gl_VertexIndex].Dx.x * ubo.lamda),
		pos.y +  HtBuffers[gl_VertexIndex].H_y.x,
		pos.z + (HtBuffers[gl_VertexIndex].Dz.x * ubo.lamda),
		1.0);

	//vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal)

	fragColor = color;
}