#pragma once
#include "RenderResources.h"

namespace star::renderer
{
	class BindSharedResourcesNode
	{
	public:
		void Execute(RenderContext& context);
	};
}
