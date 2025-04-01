#include "pch.h"
#include "VoidLight.h"

namespace star::game
{
	star::game::Light::Light() : Entity("Light")
	{

	}

	Light::Light(const std::string& type) : Entity(type)
	{
		
	}

	Light::Light(const Light& other) : Entity(other)
	{
		
	}

	std::shared_ptr<Object> Light::DeepClone()
	{
		return std::make_shared<Light>(*this);
	}
}
