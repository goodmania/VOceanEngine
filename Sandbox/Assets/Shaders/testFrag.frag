#version 450

layout(location = 0) in vec4 fragWorldPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragWorldNormal;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push
{
	mat4 ModelMatrix; 
	mat4 NormalMatrix;
} push;

layout(std140, set = 0, binding = 2) uniform GlobalUBO
{
	mat4 ProjectionView;
	vec3 LightDirection;
	vec3 SeaBaseColor;
	float BaseColorStrength;
	vec3 SeaShallowColor;
	float ColorHeightOffset;
	vec3 CameraPos;
} globalUbo;

layout(std430, set = 0, binding = 3) buffer OceanNormalBuffer
{
	vec4 OceanNormalBuffers[];
};

const float AMBIENT = 1.0f;

vec3 GetSkyColor(vec3 refrectDir, vec3 skyColor)
{;
	refrectDir.y = max(0.0, refrectDir.y);
	return (1.0 - skyColor) * (1.0 - refrectDir.y) + skyColor;
}

void main()
{
	//float lightIntensity = AMBIENT + max(dot(normalize(fragWorldNormal.xyz), normalize(globalUbo.LightDirection)), 0);
	vec3 normal = normalize(fragWorldNormal.xyz);
	vec3 viewDir = normalize(globalUbo.CameraPos.xyz - fragWorldPos.xyz);
	vec3 reflectDir = reflect(viewDir, normal);
	vec3 skyColor = normalize(vec3(0, 104, 255));
	vec3 oceanReflectColor = GetSkyColor(reflectDir, skyColor);
	
	//fresnel
	float r = 0.02f;
	float facing = clamp(1.0 - dot(normal, viewDir), 0.0f, 1.0f);
	float fresnel = r + (1.0 - r) * pow(facing, 5.0);

	float diff = clamp(dot(normal, normalize(globalUbo.LightDirection.xyz)), 0.f, 1.0f) + AMBIENT;
	float heightOffset = (fragWorldPos.y * 0.002f + 0.2f) * globalUbo.ColorHeightOffset;

	vec3 oceanBaseColor = globalUbo.SeaBaseColor * diff * globalUbo.BaseColorStrength;
	vec3 waterColor = mix(oceanBaseColor, oceanReflectColor, fresnel);
	vec3 oceanColor = waterColor/* + globalUbo.SeaShallowColor * heightOffset*/;

	outColor = vec4(oceanColor, 1.0f);
}
