#pragma once

#include "ResourceHandles.h"
#include "VoidBuffers.h"
#include <glm/mat4x4.hpp>
#include <memory>

namespace star::graphics
{
	struct DrawConstants
	{
		glm::mat4 localToWorld;
		int hasNormal;
		int hasMetalness;
		int hasRoughness;
		int hasAlpha;
		uint32_t meshletCount;
	};

	struct RenderProxy
	{
		resource::MeshHandle meshHandle;
		glm::mat4 worldMatrix;
		std::shared_ptr<VoidStructuredBuffer> constants;
	};
}