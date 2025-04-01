#pragma once

namespace star::graphics
{
	class VoidRenderNode
	{
	public:
		virtual ~VoidRenderNode() = default;
		virtual void Execute(IVoidGraphicsCommandList* commandList) = 0;
	};
}