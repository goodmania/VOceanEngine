#version 450

layout(location = 0) in vec4 fragWorldPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragWorldNormal;
layout(location = 3) in vec2 fragTexCoords;

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
	vec3 OceanNormalBuffers[];
};

//layout(binding = 4, r32f) uniform readonly image2D OceanBubbleImage;

vec3 GetSkyColor(vec3 refrectDir, vec3 skyColor)
{
	refrectDir.y = max(0.0, refrectDir.y);
	return (1.0 - skyColor) * (1.0 - refrectDir.y) + skyColor;
}

void main()
{
	//float lightIntensity = AMBIENT + max(dot(normalize(fragWorldNormal.xyz), normalize(globalUbo.LightDirection)), 0);
	vec3 normal = normalize(fragWorldNormal);
	vec3 viewDir = normalize(globalUbo.CameraPos.xyz - fragWorldPos.xyz);
	vec3 reflectDir = reflect(-viewDir, normal.xyz);
	//vec3 skyColor = vec3(1.0f, 1.0f, 1.0f);
	vec3 skyColor = vec3(105.0f / 256, 133.0f / 256, 184.0f/ 256);
	vec3 oceanReflectColor = GetSkyColor(reflectDir, skyColor);

	//fresnel
	float r = 0.02f;
	float facing = clamp(1.0f + dot(normal.xyz, viewDir), 0.0f, 1.0f);
	float fresnel = r + (1.0f - r) * pow(facing, 5.0f);

	float diffuse = clamp(dot(normal.xyz, normalize(globalUbo.LightDirection)), 0.0f, 1.0f);
	float heightOffset = (fragWorldPos.y  * 0.01f + 0.2f) * globalUbo.ColorHeightOffset;

	vec3 oceanBaseColor = globalUbo.SeaBaseColor * diffuse * globalUbo.BaseColorStrength;
	vec3 waterColor = mix(oceanBaseColor, oceanReflectColor, fresnel);
	vec3 oceanColor = waterColor + globalUbo.SeaShallowColor * heightOffset;

	ivec2 texCoords = ivec2(fragTexCoords.xy);

	/*vec4 bubble = imageLoad(OceanBubbleImage, texCoords);

	if (bubble.x < -0.3f)
	{
		bubble.x = min(-bubble.x * 0.4, 1);
		oceanColor = bubble.x * vec3(1.0f, 1.0f, 1.0f) + (1.0f - bubble.x) * oceanColor;
	}*/

	outColor = vec4(oceanColor, 1.0f);
}
