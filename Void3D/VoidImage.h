#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>

enum VoidResourceFlags
{
	None,
	RenderTarget,
	DepthStencil,
	UnorderedAccess
};

class VoidImage
{
public:

	VoidImage(uint32_t width, uint32_t height, uint32_t depth)
	{
		_width = width;
		_height = height;
		_depth = depth;
	}

	uint32_t get_width() const
	{
		return _width;
	}

	uint32_t get_height() const
	{
		return _height;
	}

	uint32_t get_depth() const
	{
		return _depth;
	}

	virtual void poly(){}

protected:
	uint32_t _width;
	uint32_t _height;
	uint32_t _depth;
};
