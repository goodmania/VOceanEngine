#include "PreCompileHeader.h"
#include "CameraController.h"

#include "VOceanEngine/Input.h"
#include "VOceanEngine/keyCodes.h"

#include "Renderer/GameObject.h"
#include "Renderer/Camera.h"

namespace voe {

	

	void CameraController::OnUpdate(float dt, GameObject& gameObject)
	{
		glm::vec3 rotate{ 0.f };

		m_CurrentMousePos = m_NewMousePos;
		m_NewMousePos = glm::vec2(Input::GetMouseX(), Input::GetMouseY());
		
		if (m_NewMousePos.x > m_CurrentMousePos.x)
			rotate.y += 0.05f;
		else if (m_NewMousePos.x < m_CurrentMousePos.x)
			rotate.y -= 0.05f;

		if (m_NewMousePos.y > m_CurrentMousePos.y)
			rotate.x -= 0.1;
		else if (m_NewMousePos.y < m_CurrentMousePos.y)
			rotate.x += 0.1;

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) 
		{
			gameObject.m_Transform.Rotation += m_LookSpeed * dt * glm::normalize(rotate);
		}

		// limit pitch values between about +/- 85ish degrees
		gameObject.m_Transform.Rotation.x = glm::clamp(gameObject.m_Transform.Rotation.x, -1.5f, 1.5f);
		gameObject.m_Transform.Rotation.y = glm::mod(gameObject.m_Transform.Rotation.y, glm::two_pi<float>());

		float yaw = gameObject.m_Transform.Rotation.y;
		const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };
		// forwardDir X upDir
		const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
		
		glm::vec3 moveDir{ 0.f };
		if (Input::IsKeyPressed(Key::W)) moveDir += forwardDir;
		else if (Input::IsKeyPressed(Key::S)) moveDir -= forwardDir;

		if (Input::IsKeyPressed(Key::D)) moveDir += rightDir;
		else if (Input::IsKeyPressed(Key::A)) moveDir -= rightDir;
		
		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
		{
			gameObject.m_Transform.Translation += m_MoveSpeed * dt * glm::normalize(moveDir);
		}
	}
}