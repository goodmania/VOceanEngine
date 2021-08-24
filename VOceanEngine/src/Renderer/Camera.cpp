#include "PreCompileHeader.h"
#include "Camera.h"

namespace voe {

	Camera::Camera()
	{

	}

	Camera::~Camera()
	{

	}

	glm::mat4 Camera::CreateOrthoProjectionMatrix(float l, float r, float t, float b, float n, float f)
	{		
		const float inverseW = 1.0f / (r - l); // inverse view volume width
		const float inverseH = 1.0f / (b - t); // inverse view volume height
		const float inverseD = 1.0f / (f - n); // inverse view volume depth

		return glm::mat4{
			glm::vec4(2.0f * inverseW, 0.0f, 0.0f, -(r + l) * inverseW),
			glm::vec4(0.0f, 2.0f * inverseH, 0.0f, -(b + t) * inverseH),
			glm::vec4(0.0f, 0.0f, inverseD, -n * inverseD),
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) };
	}

	void Camera::SetFrustumProjectionMatrix(float fovy, float aspect, float n, float f)
	{
		VOE_ASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		const float g = 1.0f / glm::tan(fovy * 0.5f); // Distance from camera to projection plane
		const float k = f / (f - n);

		
		m_ProjectionMatrix = glm::mat4{ 0.0f };
		m_ProjectionMatrix = glm::mat4{
			g / aspect,	0.0f,	0.0f,		0.0f,
			0.0f,		g	,	0.0f,		0.0f,
			0.0f,		0.0f,	k,			1.0f,
			0.0f,		0.0f,	-n * k,		0.0f};
	}

	glm::mat4 Camera::CreateViewMatrix(glm::vec3 pos, glm::vec3 target, glm::vec3 up)
	{
		const glm::vec3 dir(target - pos);

		const glm::vec3 zAxis(glm::normalize(dir));
		const glm::vec3 xAxis(glm::normalize(cross(zAxis, up)));
		const glm::vec3 yAxis(glm::normalize(cross(zAxis, xAxis)));

		return glm::mat4{
			xAxis.x,	xAxis.y,	xAxis.z,	-glm::dot(xAxis, pos),
			yAxis.x,	yAxis.y,	yAxis.z,	-glm::dot(yAxis, pos),
			zAxis.x,	zAxis.y,	zAxis.z,	-glm::dot(zAxis, pos),
			0.0f,		0.0f,		0.0f,		1.0f };
	}

	void Camera::SetViewYXZ(glm::vec3 pos, glm::vec3 rotation)
	{
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
		const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
		const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };

		m_ViewMatrix = glm::mat4{ 1.f };
		m_ViewMatrix = glm::mat4{
			u.x,				v.x,				w.x,				1.0f,
			u.y,				v.y,				w.y,				1.0f,
			u.z,				v.z,				w.z,				1.0f,
			-glm::dot(u, pos),	-glm::dot(v, pos),	-glm::dot(w, pos),	1.0f };
	}


	void Camera::OnUpdate()
	{

	}
}

