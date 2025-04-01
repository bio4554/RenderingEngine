#pragma once
#include <optional>
#include "VoidAssertUtil.h"
#include "D3D12MemAlloc.h"
#include "VoidResource.h"

class VoidRenderContext_D3D12;

class D3D12_VoidResource : public VoidResource
{
public:
	friend class VoidRenderContext_D3D12;
	friend class D3D12_PipelineState;
	friend class VoidGraphicsCommandList_D3D12;

	D3D12_VoidResource(D3D12MA::Allocation* alloc, ID3D12Resource* resource, VoidRenderContext_D3D12* context)
	{
		_allocation = alloc;
		_d3d12Resource = resource;
		_currentState = D3D12_RESOURCE_STATE_COMMON;
		_context = context;
	}

	D3D12_VoidResource(ID3D12Resource* resource, VoidRenderContext_D3D12* context)
	{
		_d3d12Resource = resource;
		_allocation = nullptr;
		_currentState = D3D12_RESOURCE_STATE_COMMON;
		_context = context;
	}

	~D3D12_VoidResource();

	D3D12MA::Allocation* get_d3d12_allocation() const
	{
		return _allocation;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_srv() const
	{
		if(!_srvDescriptor.has_value())
		{
			auto b = 2;
		}

		VOID_ASSERT(_srvDescriptor.has_value(), "SRV not valid");

		return _srvDescriptor.value();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_rtv() const
	{
		if(!_rtvDescriptor.has_value())
		{
			throw std::runtime_error("no rtv");
		}

		VOID_ASSERT(_rtvDescriptor.has_value(), "RTV not valid");

		return _rtvDescriptor.value();
	}

	/*const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& get_d3d12_rtv_list() const
	{
		VOID_ASSERT(_rtvDescriptors.has_value(), "RTV not valid");

		return *_rtvDescriptors;
	}

	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& get_d3d12_dsv_list() const
	{
		VOID_ASSERT(_dsvDescriptors.has_value(), "DSV not valid");

		return *_dsvDescriptors;
	}*/

	D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_dsv() const
	{
		VOID_ASSERT(_dsvDescriptor.has_value(), "DSV not valid");

		return _dsvDescriptor.value();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_cbv() const
	{
		VOID_ASSERT(_cbvDescriptor.has_value(), "CBV not valid");

		return _cbvDescriptor.value();
	}

	ID3D12Resource* get_d3d12_resource() const
	{
		return _d3d12Resource;
	}

	void dispose();

private:
	VoidRenderContext_D3D12* _context = nullptr;
	bool _disposed = false;

protected:
	D3D12MA::Allocation* _allocation;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> _srvDescriptor;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> _rtvDescriptor;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> _dsvDescriptor;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> _cbvDescriptor;
	//std::optional<std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>> _dsvDescriptors;
	//std::optional<std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>> _rtvDescriptors;
	ID3D12Resource* _d3d12Resource;
#ifdef _DEBUG
	std::optional<D3D12_RESOURCE_STATES> _currentState;
#else
	D3D12_RESOURCE_STATES _currentState;
#endif

	D3D12_RESOURCE_DESC _resourceDesc;
};

#ifdef _DEBUG
#define VOID_ASSERT_STATE_SET(resource) \
if(!resource->_currentState.has_value()) \
	throw std::runtime_error("_currentState not set")
#else
#define VOID_ASSERT_STATE_SET(resource) ((void)0)
#endif

#ifdef _DEBUG
#define VOID_GET_RESOURCE_STATE(resource) \
resource->_currentState.value()
#else
#define VOID_GET_RESOURCE_STATE(resource) \
resource->_currentState
#endif