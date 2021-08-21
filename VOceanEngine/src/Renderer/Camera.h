#pragma once

#include <glm/glm.hpp>

namespace voe {

	class Camera
	{
	public:
		Camera(float l, float r, float t, float b, float n, float f);
		~Camera();

		glm::mat4 CreateOrthoProjectionMatrix(float l, float r, float t, float b, float n, float f);
		glm::mat4 CreateFrustrumProjectionMatrix(float fovy, float s, float n, float f);
		glm::mat4 CreateViewMatrix(glm::vec3 pos, glm::vec3 target, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));

		// talt-bryan angles yxz
		void SetViewXYZ(glm::vec3 pos, glm::vec3 rotation);

		const glm::vec3 GetCameraPos() { return m_CameraPos; }
		const float GetCameraRotation(){}

		const glm::mat4 GetProjectionMatrix() { return m_ProjectionMatrix; }
		const glm::mat4 GetViewMatrix() { return m_ViewMatrix; }

		void OnUpdate();

	private:
		glm::vec3 m_CameraPos = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraDirection;

		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
	};
}
