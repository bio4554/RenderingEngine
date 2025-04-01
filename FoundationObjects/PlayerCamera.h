#pragma once
#include "Camera.h"

namespace star::foundation
{
	class PlayerCamera : public game::Camera
	{
	public:
		PlayerCamera();
		PlayerCamera(const PlayerCamera& other);

		std::shared_ptr<Object> DeepClone() override;

		void Update(float delta) override;

	private:
		void HandleMove(glm::vec3 moveVector);
		void HandleMouse(glm::vec2 delta);

		float _speed;
		bool _freeLook;
	};
}
