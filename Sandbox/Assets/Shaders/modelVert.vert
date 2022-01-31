#version 450

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 vertTexCoords;

layout(location = 0) out vec4 fragWorldPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) out vec2 fragTexCoords;

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
	mat4 NormalMatrix;
} push;

layout(set = 0, binding = 0) uniform ModelUBO
{
	mat4 ProjectionViewMatrix;
	vec4 AmbientLightColor; // w is intensity
	vec3 LightPosition;
	vec4 LightColor;
} ubo;

void main()
{
	vec4 positionWorld = push.ModelMatrix * pos;
	gl_Position = ubo.ProjectionViewMatrix * positionWorld;
	fragWorldNormal = normalize(mat3(push.NormalMatrix) * normal);
	fragWorldPos = positionWorld;
	fragColor = color;
}