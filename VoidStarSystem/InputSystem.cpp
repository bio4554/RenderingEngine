#include "pch.h"
#include "InputSystem.h"

#include <SDL_events.h>

#include "imgui_impl_sdl2.h"

namespace star::system
{
    InputSystem* GInputSystem = nullptr;

	InputSystem::InputSystem()
	{
		ResetState();
	}

	void InputSystem::ProcessEvents()
	{
		SDL_Event event;

        _mouseDelta = { 0.f, 0.f };

		while (SDL_PollEvent(&event) != 0) {
			// User requests quit
			if (event.type == SDL_QUIT) {
				_inputState.buttons[InputButton::Quit] = ButtonState::Down;
			}

            ImGui_ImplSDL2_ProcessEvent(&event);

            HandleEvent(&event);
		}
	}

	void InputSystem::ResetState()
	{
		_inputState = InputState();

		auto count = static_cast<uint32_t>(InputButton::Count);

		for(uint32_t i = 0; i < count; i++)
		{
			_inputState.buttons[static_cast<InputButton>(i)] = ButtonState::Up;
		}
	}

	void InputSystem::HandleEvent(const SDL_Event* event)
	{
        if (event->type == SDL_KEYDOWN) {
            if (event->key.keysym.sym == SDLK_w) { _inputState.buttons[InputButton::W] = ButtonState::Down; }
            if (event->key.keysym.sym == SDLK_s) { _inputState.buttons[InputButton::S] = ButtonState::Down; }
            if (event->key.keysym.sym == SDLK_a) { _inputState.buttons[InputButton::A] = ButtonState::Down; }
            if (event->key.keysym.sym == SDLK_d) { _inputState.buttons[InputButton::D] = ButtonState::Down; }

            if (event->key.keysym.sym == SDLK_ESCAPE) { _inputState.buttons[InputButton::Escape] = ButtonState::Down; }
        }

        if (event->type == SDL_KEYUP) {
            if (event->key.keysym.sym == SDLK_w) { _inputState.buttons[InputButton::W] = ButtonState::Up; }
            if (event->key.keysym.sym == SDLK_s) { _inputState.buttons[InputButton::S] = ButtonState::Up; }
            if (event->key.keysym.sym == SDLK_a) { _inputState.buttons[InputButton::A] = ButtonState::Up; }
            if (event->key.keysym.sym == SDLK_d) { _inputState.buttons[InputButton::D] = ButtonState::Up; }
            if (event->key.keysym.sym == SDLK_ESCAPE) { _inputState.buttons[InputButton::Escape] = ButtonState::Up; }
        }

        if (event->type == SDL_MOUSEMOTION) {
            _mouseDelta += glm::vec2{ (float)event->motion.xrel, (float)event->motion.yrel };
        }

        if (event->type == SDL_MOUSEBUTTONDOWN)
        {
            if (event->button.button == SDL_BUTTON_RIGHT)
            {
                _inputState.buttons[InputButton::RightMouse] = ButtonState::Down;
            }
            else if (event->button.button == SDL_BUTTON_LEFT)
            {
                _inputState.buttons[InputButton::LeftMouse] = ButtonState::Down;
            }
        }

        if (event->type == SDL_MOUSEBUTTONUP)
        {
            if (event->button.button == SDL_BUTTON_RIGHT)
            {
                _inputState.buttons[InputButton::RightMouse] = ButtonState::Up;
            }
            else if (event->button.button == SDL_BUTTON_LEFT)
            {
                _inputState.buttons[InputButton::LeftMouse] = ButtonState::Up;
            }
        }
	}


}
