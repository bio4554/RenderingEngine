#include "pch.h"
#include "BindShadowMapsNode.h"

namespace star::renderer
{
	void BindShadowMapsNode::Execute(RenderContext& context)
	{
		auto shadowCount = context.frameData.shadowMaps.size();
		assert(shadowCount < static_cast<size_t>(std::numeric_limits<uint16_t>::max()));
		for(size_t i = 0; i < shadowCount; i++)
		{
			auto shadow = context.frameData.shadowMaps[i];
			context.context->bind_resource(32 + static_cast<uint16_t>(i), shadow);
		}
	}

}