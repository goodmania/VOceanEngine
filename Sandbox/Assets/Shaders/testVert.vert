#version 450

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 normal;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform Push 
{
  mat4 ModelMatrix; // projection * view * model
  mat4 normalMatrix;
} push;

layout(std430, set = 0, binding = 0) buffer HtBuffer
{
	vec2 HtBuffers[];
};

layout(std140, set = 0, binding = 1) uniform UBO
{
	float deltaT;
	float lambda;
	highp uint meshSize;
	highp uint OceanSizeLx;
	highp uint OceanSizeLz;
} ubo;

layout(std140, set = 0, binding = 2) uniform GlobalUBO
{
	mat4 ProjectionView;
	vec3 LightDirection;
	vec3 SeaBaseColor;
	float BaseColorStrength;
	vec3 SeaShallowColor;
	float ColorHightOffset;
} globalUbo;

layout(std430, set = 0, binding = 3) buffer OceanNormalBuffer
{
	vec4 OceanNormalBuffers[];
};

const float AMBIENT = 0.02f;

void main()
{
	uint offset = ubo.meshSize * ubo.meshSize;
	vec4 positionWorld = push.ModelMatrix * vec4(
		pos.x + (HtBuffers[gl_VertexIndex + offset * 3].y * ubo.lambda), // dx
		pos.y +  HtBuffers[gl_VertexIndex + offset * 0].x,
		pos.z + (HtBuffers[gl_VertexIndex + offset * 4].y * ubo.lambda), // dz
		1.0);

	gl_Position = globalUbo.ProjectionView * positionWorld;

	vec4 normalWorldSpace = normalize(push.normalMatrix * OceanNormalBuffers[gl_VertexIndex]);
	float lightIntensity = AMBIENT + max(dot(normalWorldSpace, vec4(globalUbo.LightDirection, 0.0f)), 0);

	vec3 seaColor = lightIntensity * globalUbo.SeaBaseColor * globalUbo.BaseColorStrength
		+ (globalUbo.SeaShallowColor * (1.0f * 0.5f + 0.2) * globalUbo.ColorHightOffset);

	fragColor = vec4(seaColor, 1.0f);
}