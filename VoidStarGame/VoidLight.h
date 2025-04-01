#pragma once
#include "VoidEntity.h"

namespace star::game
{
	class Light : public Entity
	{
	public:
		enum class Type
		{
			Directional
		};

		Light();
		Light(const std::string& type);
		Light(const Light& other);

		std::shared_ptr<Object> DeepClone() override;

		Type type;
		glm::vec3 direction;
		glm::vec3 color;
		float intensity;
	};
}
