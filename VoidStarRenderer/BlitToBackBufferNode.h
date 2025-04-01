#pragma once
#include "RenderResources.h"

namespace star::renderer
{
	class BlitToBackBufferNode
	{
	public:
		void Execute(RenderContext& context);
	};
}
