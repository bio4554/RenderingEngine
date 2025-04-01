#pragma once

#include <vector>

#include "VoidImage.h"
#include "ResourceHandles.h"

namespace star::graphics
{
	class Texture
	{
	public:
		Texture()
		{
			this->handle = resource::TextureHandle(std::numeric_limits<resource::Handle>::max());
			width = 0;
			height = 0;
		}

		std::shared_ptr<VoidImage> imageResource;
		std::vector<uint8_t> imageData;
		uint32_t width;
		uint32_t height;
		resource::TextureHandle handle;
	};
}