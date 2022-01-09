#pragma once

#include "Renderer/Model.h"
#include <glm/gtc/matrix_transform.hpp>

namespace voe {

	struct TransformComponent {
		glm::vec3 Translation{};
		glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
		glm::vec3 Rotation{};

		// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 Mat4();
		glm::mat3 NormalMatrix();
	};

	struct Transform2dComponent {
		glm::vec2 Translation{};  // (position offset)
		glm::vec2 Scale{ 1.f, 1.f };
		float Rotation{};

		glm::mat2 Mat2() 
		{
			const float s = glm::sin(Rotation);
			const float c = glm::cos(Rotation);
			glm::mat2 RotMatrix{ {c, s}, {-s, c} };

			glm::mat2 ScaleMat{ {Scale.x, 0.0f}, {0.0f, Scale.y} };
			return RotMatrix * ScaleMat;
		}
	};

	class VOE_API GameObject
	{
	public:
		using id_t = uint32_t;

		static GameObject CreateGameObject()
		{
			static id_t currentId = 0;
			return GameObject{ currentId++ };
		}

		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = default;
		GameObject& operator=(GameObject&&) = default;

		id_t GetId() { return m_Id; }

		std::shared_ptr<Model> m_Model{};
		glm::vec3 m_Color{};
		TransformComponent m_Transform{};
		Transform2dComponent m_Transform2d{};

	private:
		GameObject(id_t objId) : m_Id{objId} {}

		id_t m_Id;
	};
}



