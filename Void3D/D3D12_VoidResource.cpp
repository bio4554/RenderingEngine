#include "pch.h"
#include "VoidRenderContext_D3D12.h"
#include "D3D12_VoidResource.h"

D3D12_VoidResource::~D3D12_VoidResource()
{
	dispose();
}

void D3D12_VoidResource::dispose()
{
	if(!_disposed)
	{
		if(_context != nullptr)
		{
			_context->release_resource(this);
		}
		else if(_srvDescriptor.has_value())
		{
			auto b = 2;
			//throw std::runtime_error("context null");
		}

		_disposed = true;
	}
}
