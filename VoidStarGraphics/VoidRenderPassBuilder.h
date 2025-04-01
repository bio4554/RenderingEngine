#pragma once
#include <functional>
#include <stdexcept>

#include "IVoidGraphicsCommandList.h"
#include "IVoidRenderContext.h"
#include "JobSystem.h"
#include "SyncCounter.h"
#include "VoidRenderNode.h"

namespace star::graphics
{
	class VoidRenderPassBuilder
	{
	public:
		VoidRenderPassBuilder(const std::string& name, IVoidRenderContext* context);
		VoidRenderPassBuilder(const std::string& name, IVoidRenderContext* context, std::shared_ptr<core::SyncCounter> counter);
		VoidRenderPassBuilder(const std::string& name, IVoidRenderContext* context, core::JobBuilder& builder);

		void AddAnonymous(const std::string& name, const std::function<void(std::shared_ptr<IVoidGraphicsCommandList>)>& func);

		template<typename T>
		void AddNode(std::string name)
		{
			static_assert(std::is_base_of_v<VoidRenderNode, T>, "T must be derived from VoidRenderNode");

			_jobs.push_back(std::pair(name, [](IVoidGraphicsCommandList* list)
				{
					T node = T();
					node.Execute(list);
				}));
		}

		std::shared_ptr<core::SyncCounter> Dispatch() const;

	private:
		IVoidRenderContext* _context;
		std::string _name;
		//IVoidGraphicsCommandList* _commandList;
		std::vector<std::pair<std::string, std::function<void(std::shared_ptr<IVoidGraphicsCommandList>)>>> _jobs;
		std::shared_ptr<core::SyncCounter> _syncCounter;
	};
}
