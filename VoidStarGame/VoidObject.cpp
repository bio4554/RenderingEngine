#include "pch.h"
#include "VoidObject.h"

namespace star::game
{
	Object::Object()
	{
		type = "Object";
	}

	Object::Object(const Object& other)
	{
		type = other.type;

		// todo how slow is this?
		for(const auto& child : other.children)
		{
			children.push_back(child->DeepClone());
		}
	}

	std::shared_ptr<Object> Object::DeepClone()
	{
		return std::make_shared<Object>(*this);
	}

	void Object::Update(float delta)
	{
	}

	Object::Object(const std::string& type)
	{
		this->type = type;
	}
}
