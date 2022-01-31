#version 450

layout(location = 0) in vec4 fragWorldPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragWorldNormal;
layout(location = 3) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 ProjectionViewMatrix;
	vec4 AmbientLightColor; // w is intensity
	vec3 LightPosition;
	vec4 LightColor;
} ubo;

void main() 
{
	vec3 directionToLight = ubo.LightPosition - fragWorldPos.xyz;
	float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

	vec3 lightColor = ubo.LightColor.xyz * ubo.LightColor.w * attenuation;
	vec3 ambientLight = ubo.AmbientLightColor.xyz * ubo.AmbientLightColor.w;
	vec3 diffuseLight = lightColor * max(dot(normalize(fragWorldNormal), normalize(directionToLight)), 0);
	outColor = vec4((diffuseLight + ambientLight) * fragColor.xyz, 1.0);
}