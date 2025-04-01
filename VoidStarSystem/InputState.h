#pragma once
#include <unordered_map>

namespace star::system
{
	enum class InputButton : uint32_t
	{
		W = 0,
		A,
		S,
		D,
		Escape,
		RightMouse,
		LeftMouse,
		Quit,
		Count
	};

	enum class ButtonState : uint8_t
	{
		Down,
		Up
	};

	struct InputState
	{
		std::unordered_map<InputButton, ButtonState> buttons;
	};
}
