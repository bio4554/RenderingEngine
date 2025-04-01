#pragma once
#include "RenderResources.h"

namespace star::renderer
{
	class ClearTargetNode
	{
	public:
		void Execute(RenderContext& context, std::string name);
	};
}
