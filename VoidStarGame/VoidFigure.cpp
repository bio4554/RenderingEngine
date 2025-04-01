#include "pch.h"
#include "VoidFigure.h"

namespace star::game
{
	Figure::Figure() : Entity("Figure")
	{
	}

	Figure::Figure(const std::string& type) : Entity(type)
	{
	}

	Figure::Figure(const Figure& other) : Entity(other), meshHandle(other.meshHandle)
	{
	}

	std::shared_ptr<Object> Figure::DeepClone()
	{
		return std::make_shared<Figure>(*this);
	}

}