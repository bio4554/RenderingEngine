#include "pch.h"
#include "VoidRenderContext_D3D12.h"
#include "D3D12_VoidSampler.h"

D3D12_VoidSampler::~D3D12_VoidSampler()
{
	free();
}

void D3D12_VoidSampler::free()
{
	if(!_disposed)
	{
		if(_context != nullptr)
		{
			_context->release_sampler(this);
		}

		_disposed = true;
	}
}

