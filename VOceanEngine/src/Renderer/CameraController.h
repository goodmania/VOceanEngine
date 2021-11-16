#pragma once

namespace voe {

	class Camera;
	class GameObject;

	class CameraController
	{
	public:
		CameraController() = default;
		void OnUpdate(float dt, GameObject& gameObject);

	private:
		float m_MoveSpeed{ 100.f };
		float m_LookSpeed{ 1.5f };

		glm::vec2 m_CurrentMousePos{ 0.f, 0.f };
		glm::vec2 m_NewMousePos{ 0.f, 0.f };
	};
}



