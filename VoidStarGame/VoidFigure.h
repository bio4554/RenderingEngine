#pragma once

#include "ResourceHandles.h"
#include "VoidEntity.h"

namespace star::game
{
	class Figure : public Entity
	{
	public:
		Figure();
		Figure(const std::string& type);
		Figure(const Figure& other);

		std::shared_ptr<Object> DeepClone() override;

		std::optional<resource::MeshHandle> meshHandle;
	};
}