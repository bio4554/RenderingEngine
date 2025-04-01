#include "pch.h"
#include "Camera.h"

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtx/quaternion.hpp>

namespace star::game
{
	Camera::Camera() : Entity("Camera"), position(), pitch(0), yaw(0), fov(0), near(0), far(0), aspectRatio(0)
	{
	}

	Camera::Camera(const Camera& other) : Entity(other), position(other.position), pitch(other.pitch), yaw(other.yaw), fov(other.fov),
	                                      near(other.near), far(other.far), aspectRatio(other.aspectRatio)
	{
	}

	std::shared_ptr<game::Object> Camera::DeepClone()
	{
		auto camera = std::make_shared<Camera>(*this);
		return camera;
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
		glm::mat4 cameraRotation = GetRotationMatrix();
		return glm::inverse(cameraTranslation * cameraRotation);
	}

	glm::mat4 Camera::GetRotationMatrix() const
	{
		glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
		glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });

		return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
	}

	glm::mat4 Camera::GetProjectionMatrix() const
	{
		const auto proj = glm::perspective(glm::radians(fov), aspectRatio, near, far);
		return proj;
	}

}
