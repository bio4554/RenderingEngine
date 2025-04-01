#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace star::core
{
	struct Transform
	{
		glm::vec3 translation;
		glm::vec3 rotation;
		glm::vec3 scale;

		[[nodiscard]] glm::mat4 GetMatrix() const
		{
			glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), translation);
			glm::mat4 rotXMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rotYMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 rotZMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 rotMat = rotZMat * rotYMat * rotXMat; // Z * Y * X order
			glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

			auto transform = translateMat * rotMat * scaleMat;

			return transform;
		}
	};
}
