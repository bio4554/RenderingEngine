#include "pch.h"
#include "PlayerCamera.h"

#include "InputSystem.h"


namespace star::foundation
{
	PlayerCamera::PlayerCamera() : Camera(), _speed(10.f), _freeLook(false)
	{
	}

	PlayerCamera::PlayerCamera(const PlayerCamera& other) : Camera(other), _speed(other._speed), _freeLook(other._freeLook)
	{
		
	}

	std::shared_ptr<game::Object> PlayerCamera::DeepClone()
	{
		auto cam = std::make_shared<PlayerCamera>(*this);
		return cam;
	}

	void PlayerCamera::Update(float delta)
	{
		auto moveDir = glm::vec3(0.f);

		if(system::GInputSystem->Is(system::InputButton::W, system::ButtonState::Down))
		{
			moveDir.z = -1.f;
		}
		if(system::GInputSystem->Is(system::InputButton::S, system::ButtonState::Down))
		{
			moveDir.z = 1.f;
		}
		if(system::GInputSystem->Is(system::InputButton::A, system::ButtonState::Down))
		{
			moveDir.x = -1.f;
		}
		if(system::GInputSystem->Is(system::InputButton::D, system::ButtonState::Down))
		{
			moveDir.x = 1.f;
		}

		_freeLook = system::GInputSystem->Is(system::InputButton::RightMouse, system::ButtonState::Down);

		HandleMouse(system::GInputSystem->MouseDelta());
		HandleMove(moveDir);
	}

	void PlayerCamera::HandleMove(glm::vec3 moveVector)
	{
		glm::mat4 rotation = GetRotationMatrix();
		auto velocity = moveVector * _speed;
		position += glm::vec3(rotation * glm::vec4(velocity * 0.5f, 0.f));
	}

	void PlayerCamera::HandleMouse(glm::vec2 delta)
	{
		if (!_freeLook)
			return;
		yaw += delta.x / 200.f;
		pitch -= delta.y / 200.f;

		pitch = std::clamp(pitch, -1.5f, 1.5f);
	}

}
