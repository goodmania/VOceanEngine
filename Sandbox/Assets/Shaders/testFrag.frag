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
	float SeaShallowColorStrength;
	vec3 CameraPos;
} globalUbo;

layout(binding = 3, rgba32f) uniform readonly image2D OceanNormalImage;

layout(binding = 4, r32f) uniform readonly image2D OceanBubbleImage;

vec3 GetSkyColor(vec3 refrectDir, vec3 skyColor)
{
	refrectDir.y = max(0.0, -refrectDir.y);
	float et = 1.0 - refrectDir.y;
	return (vec3(1.0f, 1.0f, 1.0f) - skyColor) * et + skyColor;
}

void main()
{
	vec3 normal = normalize(fragWorldNormal);
	vec3 fragPos = normalize(fragWorldPos.xyz / fragWorldPos.w);
	float waveHeight = fragPos.y;
	const float COLOR_HEIGHT_OFFSET = 0.15f;

	vec3 cameraPos = normalize(vec3(globalUbo.CameraPos.x, globalUbo.CameraPos.y, globalUbo.CameraPos.z));
	vec3 viewDir = normalize(-fragPos + cameraPos);
	vec3 reflectDir = reflect(-viewDir, normal.xyz);

	vec3 skyColor = vec3(0.0f / 255.0f, 104.f / 255.0f, 255.f / 255.0f);
	//vec3 skyColor =vec3(1.0f, 1.0f, 1.0f);

	// fresnel
	float r = 0.02f;
	float facing = clamp(dot(normal, viewDir), 0.0f, 1.0f);
	float fresnel = r + (1.0f - r) * pow(1.0f - facing, 5.0f);

	// diffuse
	float diffuse = clamp(dot(normal.xyz, normalize(globalUbo.LightDirection)), 0.0f, 1.0f);

	// height offset(Change the ocean color based on the height).
	float heightOffset = (-waveHeight * 0.5f + 0.2f) * COLOR_HEIGHT_OFFSET;

	vec3 oceanReflectColor = GetSkyColor(reflectDir, skyColor);
	vec3 oceanBaseColor = globalUbo.SeaBaseColor * diffuse * globalUbo.BaseColorStrength
		+ mix(globalUbo.SeaBaseColor, globalUbo.SeaShallowColor * globalUbo.SeaShallowColorStrength, diffuse);

	vec3 waterColor = mix(oceanBaseColor, oceanReflectColor, fresnel);
	vec3 oceanColor = waterColor + globalUbo.SeaShallowColor * heightOffset;

	ivec2 texCoords = ivec2(fragTexCoords.xy);

	vec4 bubble = imageLoad(OceanBubbleImage, texCoords);

	if (bubble.x < -0.3f)
	{
		bubble.x = min(-bubble.x * 0.4f, 1.0f);
		oceanColor = bubble.x * vec3(1.0f, 1.0f, 1.0f) + (1.0f - bubble.x) * oceanColor;
	}
	outColor = vec4(waterColor, 1.0f);
}
