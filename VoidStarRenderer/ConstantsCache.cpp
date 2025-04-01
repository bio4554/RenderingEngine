#include "pch.h"
#include "ConstantsCache.h"

namespace star::renderer
{
	ConstantsCache::ConstantsCache(IVoidRenderContext* context)
	{
		_context = context;
	}

	std::shared_ptr<VoidStructuredBuffer> ConstantsCache::GetBuffer(size_t size)
	{
		if(!_buffers.contains(size))
		{
			_buffers[size] = std::vector<std::shared_ptr<VoidStructuredBuffer>>();
			_topMap[size] = 0;
		}

		auto& bufferList = _buffers[size];
		auto& top = _topMap[size];

		if(bufferList.size() > top)
		{
			return bufferList[top++];
		}

		auto buffer = _context->create_structured(size, true);
		_context->register_constant_view(buffer.get());
		bufferList.push_back(buffer);
		top++;
		return buffer;
	}

	void ConstantsCache::Reset()
	{
		for(auto& pair : _topMap)
		{
			_topMap[pair.first] = 0;
		}
	}

}
