#pragma once
#include <optional>

#include "ResourceHandles.h"

namespace star::graphics
{
	class Material
	{
	public:
		Material(resource::TextureHandle texture)
		{
			this->albedo = texture;
		}

		//Material(resource::TextureHandle texture, resource::TextureHandle normal)
		//{
		//	this->albedo = texture;
		//	this->normal = normal;
		//}

		resource::TextureHandle albedo;
		std::optional<resource::TextureHandle> normal;
		std::optional<resource::TextureHandle> metalness;
		std::optional<resource::TextureHandle> roughness;
		std::optional<resource::TextureHandle> alpha;
	};
}
