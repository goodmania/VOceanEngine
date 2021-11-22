#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout (push_constant) uniform Push
{
	mat4 transform; // projection view * model
	mat4 normalMatrix;
} push;

layout(std140, set = 0, binding = 2) uniform GlobalUBO
{
	mat4 ProjectionView;
	vec3 lightDirection;
} globalUbo;

layout(std430, set = 0, binding = 3) buffer OceanNormalBuffer
{
	vec4 OceanNormalBuffers[];
};

void main()
{
	outColor = vec4(fragColor, 1.0f);
}
