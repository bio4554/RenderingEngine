#pragma once

#include <memory>
#include "Texture.h"
#include "VoidBuffers.h"
#include "VoidGraphicsPrimitives.h"

namespace star::graphics
{
	struct MeshRenderPacket
	{
		std::shared_ptr<VoidStructuredBuffer> vertexBuffer;
		std::shared_ptr<VoidStructuredBuffer> meshletBuffer;
		uint32_t numMeshlets;
	};

	class Mesh
	{
	public:
		explicit Mesh(const resource::MaterialHandle material)
		{
			this->material = material;
			this->handle = resource::MeshHandle(std::numeric_limits<resource::Handle>::max());
		}

		std::shared_ptr<VoidStructuredBuffer> vertexBuffer;
		std::shared_ptr<VoidStructuredBuffer> meshletBuffer;
		uint32_t numMeshlets;
		std::vector<Vertex> vertices;
		std::vector<Meshlet> meshlets;
		resource::MeshHandle handle;
		resource::MaterialHandle material;
	private:

		
	};
}
