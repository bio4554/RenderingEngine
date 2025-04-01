#pragma once
#include "FrameRenderData.h"
#include "RenderResources.h"
#include "VoidWorld.h"

namespace star::renderer
{
	class CullWorldNode
	{
	public:
		void Execute(RenderContext& context);
	private:
		void CullObject(RenderContext& context, const std::shared_ptr<game::Object>& object);
	};
}
