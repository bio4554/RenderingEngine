#pragma once
#include "D3D12_VoidResource.h"
#include "VoidImage.h"

class D3D12_VoidImage : public VoidImage, public D3D12_VoidResource
{
public:
	D3D12_VoidImage(uint32_t width, uint32_t height, uint32_t depth, D3D12MA::Allocation* allocation, ID3D12Resource* resource, DXGI_FORMAT format, VoidRenderContext_D3D12* context) :
		VoidImage(width, height, depth), D3D12_VoidResource(allocation, resource, context)
	{
		_format = format;
	}

	DXGI_FORMAT get_d3d12_format() const
	{
		return _format;
	}

private:
	DXGI_FORMAT _format;
};
