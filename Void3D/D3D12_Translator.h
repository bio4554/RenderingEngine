#pragma once
#include <stdexcept>

#include "D3D12MemAlloc.h"
#include "IVoidRenderContext.h"

namespace D3D12_Translator
{
	inline D3D12_FILTER sampler_filter(VoidSamplerFilter filter)
	{
		switch(filter)
		{
		case Linear:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case Closest:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		default:
			throw std::runtime_error("out of range");
		}
	}

	inline D3D12_TEXTURE_ADDRESS_MODE address_mode(VoidSamplerAddressMode mode)
	{
		switch(mode)
		{
		case Wrap:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		default:
			throw std::runtime_error("out of range");
		}
	}

	inline DXGI_FORMAT format(VoidImageFormat format)
	{
		switch(format)
		{
		case R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case R32_TYPELESS:
			return DXGI_FORMAT_R32_TYPELESS;
		case D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		}

		throw std::runtime_error("out of range");
	}

	inline DXGI_FORMAT dsv_type_map(DXGI_FORMAT imageFormat)
	{
		switch(imageFormat)
		{
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT;
		default:
			return imageFormat;
		}
	}

	inline DXGI_FORMAT srv_type_map(DXGI_FORMAT imageFormat)
	{
		switch(imageFormat)
		{
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_R32_FLOAT;
		default:
			return imageFormat;
		}
	}

	inline uint32_t num_channels(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return 4;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 4;
		case DXGI_FORMAT_D32_FLOAT:
			return 1;
		default:
			throw std::runtime_error("out of range");
		}
	}

	inline D3D12_FILL_MODE fill_mode(VoidFillMode mode)
	{
		switch(mode)
		{
		case Solid:
			return D3D12_FILL_MODE_SOLID;
		case Wireframe:
			return D3D12_FILL_MODE_WIREFRAME;
		}

		throw std::runtime_error("out of range");
	}

	inline size_t format_element_size(DXGI_FORMAT format)
	{
		switch(format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return 16;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 8;
		case DXGI_FORMAT_D32_FLOAT:
			return 4;
		default:
			throw std::runtime_error("out of range");
		}
	}

	inline size_t format_element_size(VoidImageFormat imageFormat)
	{
		return format_element_size(format(imageFormat));
	}

	inline D3D12_RESOURCE_STATES resource_state(VoidResourceState state)
	{
		switch(state)
		{
		case VoidResourceState::Unknown:
			return D3D12_RESOURCE_STATE_COMMON;
		case VoidResourceState::ShaderRead:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case VoidResourceState::DepthWrite:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case VoidResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case VoidResourceState::CopySrc:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case VoidResourceState::CopyDst:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		}

		throw std::runtime_error("out of range");
	}

	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE topology(VoidTopologyType type)
	{
		switch(type)
		{
		case Triangle:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}

		throw std::runtime_error("out of range");
	}

	inline D3D_PRIMITIVE_TOPOLOGY input_topology(VoidInputTopologyType type)
	{
		switch(type)
		{
		case TriangleList:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}

		throw std::runtime_error("out of range");
	}

	inline D3D12_CULL_MODE cull_mode(VoidCullMode mode)
	{
		switch(mode)
		{
		case CullBack:
			return D3D12_CULL_MODE_BACK;
		case CullFront:
			return D3D12_CULL_MODE_FRONT;
		case CullNone:
			return D3D12_CULL_MODE_NONE;
		}

		throw std::runtime_error("out of range");
	}

	inline D3D12_RESOURCE_FLAGS resource_flags(VoidResourceFlags flags)
	{
		D3D12_RESOURCE_FLAGS res = D3D12_RESOURCE_FLAG_NONE;

		if((flags & VoidResourceFlags::DepthStencil) == DepthStencil)
		{
			res |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		if((flags & VoidResourceFlags::RenderTarget) == RenderTarget)
		{
			res |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if((flags & VoidResourceFlags::UnorderedAccess) == UnorderedAccess)
		{
			res |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		return res;
	}
}
