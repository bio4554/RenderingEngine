#include "pch.h"
#include "VoidWorld.h"
#include "VoidObject.h"

namespace star::game
{

	std::unique_ptr<World> World::DeepClone() const
	{
		auto newWorld = std::make_unique<World>();

		newWorld->root_object = root_object->DeepClone();

		return std::move(newWorld);
	}
}
