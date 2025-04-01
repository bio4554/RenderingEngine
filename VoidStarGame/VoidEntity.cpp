#include "pch.h"
#include "VoidEntity.h"

namespace star::game
{
	Entity::Entity() : Object("Entity"), transform()
	{
	}

	Entity::Entity(const std::string& type) : Object(type), transform()
	{
	}

	Entity::Entity(const Entity& other) : Object(other), transform(other.transform)
	{

	}

	std::shared_ptr<Object> Entity::DeepClone()
	{
		auto entity = std::make_shared<Entity>(*this);
		return entity;
	}
}