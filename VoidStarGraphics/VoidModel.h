#pragma once

#include "ResourceHandles.h"

namespace star::graphics
{
	class Model
	{
	public:
		explicit Model(std::vector<resource::MeshHandle>& meshHandles)
		{
			this->meshHandles = meshHandles;
			this->handle = resource::ModelHandle();
		}

		std::vector<resource::MeshHandle> meshHandles;
		resource::ModelHandle handle;
	};
}