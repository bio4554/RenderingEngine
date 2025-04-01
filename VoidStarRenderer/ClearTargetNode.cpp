#include "pch.h"
#include "ClearTargetNode.h"
#include "IVoidGraphicsCommandList.h"

namespace star::renderer
{
	void ClearTargetNode::Execute(RenderContext& context, std::string name)
	{
		std::shared_ptr<VoidImage> target = context.resources->Get<VoidImage>("mainDrawBuffer");
		float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
		//float clearColor[4] = { static_cast<float>(0x87) / 255.f, static_cast<float>(0xce) / 255.f, static_cast<float>(0xeb) / 255.f, 1.f};
		context.context->clear_image(clearColor, target.get());
	}
}