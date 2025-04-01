#pragma once
#include "D3D12MemAlloc.h"
#include "VoidSampler.h"

class D3D12_VoidSampler : public VoidSampler
{
public:
	D3D12_VoidSampler(D3D12_CPU_DESCRIPTOR_HANDLE handle, VoidRenderContext_D3D12* context)
	{
		_handle = handle;
	}

	~D3D12_VoidSampler();

	D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_handle()
	{
		return _handle;
	}

	void free();

private:
	D3D12_CPU_DESCRIPTOR_HANDLE _handle;
	VoidRenderContext_D3D12* _context;
	bool _disposed = false;
};