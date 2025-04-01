#pragma once

#include "VoidEntity.h"

namespace star::game
{
	class Camera : public Entity
	{
	public:
		Camera();
		Camera(const Camera& other);

		std::shared_ptr<Object> DeepClone() override;

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetRotationMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

		glm::vec3 position;
		float pitch;
		float yaw;
		float fov;
		float near;
		float far;
		float aspectRatio;
	};
}