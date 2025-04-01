#include "pch.h"
#include "ClearDepthNode.h"

namespace star::renderer
{

	void ClearDepthNode::Execute(RenderContext& context)
	{
		auto depth = context.resources->Get<VoidImage>("mainDepthBuffer");
		context.context->clear_depth(1.f, depth.get());
	}


}