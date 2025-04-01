#pragma once
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "IVoidRenderContext.h"
#include "VoidBuffers.h"

namespace star::renderer
{
	class ConstantsCache
	{
	public:
		ConstantsCache(IVoidRenderContext* context);

		std::shared_ptr<VoidStructuredBuffer> GetBuffer(size_t size);
		void Reset();
	private:
		std::unordered_map<size_t, std::vector<std::shared_ptr<VoidStructuredBuffer>>> _buffers;
		std::unordered_map<size_t, size_t> _topMap;
		IVoidRenderContext* _context;
	};
}
