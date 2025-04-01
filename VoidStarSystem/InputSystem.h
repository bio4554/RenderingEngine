#pragma once

#include <SDL_events.h>
#include <glm/vec2.hpp>

#include "InputState.h"

namespace star::system
{
	class InputSystem
	{
	public:
		InputSystem();

		void ProcessEvents();
		bool Is(const InputButton button, const ButtonState state)
		{
			return _inputState.buttons[button] == state;
		}
		glm::vec2 MouseDelta() const
		{
			return _mouseDelta;
		}

	private:
		void ResetState();
		void HandleEvent(const SDL_Event* event);

		InputState _inputState;
		glm::vec2 _mouseDelta;
	};

	extern InputSystem* GInputSystem;
}
