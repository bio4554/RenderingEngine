#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "VoidResource.h"

class VoidStructuredBuffer : public VoidResource
{
public:
	VoidStructuredBuffer(size_t size)
	{
		_size = size;
	}

	size_t get_size() const
	{
		return _size;
	}

	virtual void* map() = 0;
	virtual void unmap() = 0;
protected:
	size_t _size;
};

class VoidIndexBuffer
{
public:
	VoidIndexBuffer(const uint32_t* pIndices, const size_t count)
	{
		for(size_t i = 0; i < count; i++)
		{
			_indices.push_back(pIndices[i]);
		}
	}

	virtual void poly() {};

protected:
	std::vector<uint32_t> _indices;
};
