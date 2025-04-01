#include "pch.h"
#include "BindSharedResourcesNode.h"
#include "IVoidGraphicsCommandList.h"

namespace star::renderer
{
	void BindSharedResourcesNode::Execute(RenderContext& context)
	{
		auto buffer = context.resources->Get<VoidStructuredBuffer>("sceneConstants");
		context.context->bind_resource(2, buffer);

		auto lightBuffer = context.resources->ToBuffer(context.frameData.lights, false, false);
		context.context->bind_resource(3, lightBuffer);
	}
}
