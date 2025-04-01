#pragma once
#include <functional>
#include <span>

#include "VoidBuffers.h"
#include "d3d12.h"
#include "D3D12_VoidResource.h"

class D3D12_VoidStructuredBuffer : public VoidStructuredBuffer, public D3D12_VoidResource
{
public:
	D3D12_VoidStructuredBuffer(D3D12MA::Allocation* allocation, ID3D12Resource* resource, size_t size, D3D12_HEAP_TYPE heapType, VoidRenderContext_D3D12* context) : VoidStructuredBuffer(size), D3D12_VoidResource(allocation, resource, context)
	{
		_heapType = heapType;
	}

	void* map() override
	{
		if (_pData != nullptr)
			return _pData;

		VOID_ASSERT(_heapType == D3D12_HEAP_TYPE_UPLOAD, "can't map a GPU buffer");

		_d3d12Resource->Map(0, nullptr, &_pData);

		return _pData;
	}

	void unmap() override
	{
		if (_pData == nullptr)
			throw std::runtime_error("can't unmap buffer, not mapped");

		_d3d12Resource->Unmap(0, nullptr);

		_pData = nullptr;
	}

protected:
	D3D12_HEAP_TYPE _heapType;
	void* _pData = nullptr;
};

class D3D12_VoidIndexBuffer : public VoidIndexBuffer, public D3D12_VoidResource
{
public:
	D3D12_VoidIndexBuffer(const uint32_t* pIndices, const size_t count, D3D12MA::Allocation* allocation, D3D12_INDEX_BUFFER_VIEW view, ID3D12Resource* resource, VoidRenderContext_D3D12* context) :
		VoidIndexBuffer(pIndices, count), D3D12_VoidResource(allocation, resource, context)
	{
		_resource = resource;
		_allocation = allocation;
		_view = view;
	}

	const D3D12_INDEX_BUFFER_VIEW* get_d3d12_view() const
	{
		return &_view;
	}

	ID3D12Resource* get_d3d12_resource() const
	{
		return _resource;
	}

	D3D12MA::Allocation* get_d3d12_allocation() const
	{
		return _allocation;
	}

private:
	ID3D12Resource* _resource;
	D3D12MA::Allocation* _allocation;
	D3D12_INDEX_BUFFER_VIEW _view;
};