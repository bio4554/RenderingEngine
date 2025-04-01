#pragma once

#include "DataTypes.h"
#include "VoidObject.h"

namespace star::game
{
	class Entity : public Object
	{
	public:
		Entity();
		explicit Entity(const std::string& type);
		Entity(const Entity& other);

		std::shared_ptr<Object> DeepClone() override;

		star::core::Transform transform;
	};
}
