#pragma once
#include <memory>

#include "RenderResources.h"
#include "IVoidGraphicsCommandList.h"

namespace star::renderer
{
	class ClearDepthNode
	{
	public:
		void Execute(RenderContext& context);
	};
}
