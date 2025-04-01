#pragma once
#include <memory>

#include "Camera.h"

namespace star::game
{
	class Object;

	class World
	{
	public:
		std::shared_ptr<Object> root_object;
		std::shared_ptr<Camera> active_camera;

		std::unique_ptr<World> DeepClone() const;
	};
}
