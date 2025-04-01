#include "pch.h"
#include "VoidRenderPassBuilder.h"

#include "JobSystem.h"

namespace star::graphics
{
	VoidRenderPassBuilder::VoidRenderPassBuilder(const std::string& name, IVoidRenderContext* context)
	{
		_context = context;
		_name = name;
		_syncCounter = nullptr;
	}

	VoidRenderPassBuilder::VoidRenderPassBuilder(const std::string& name, IVoidRenderContext* context, std::shared_ptr<core::SyncCounter> counter)
	{
		_context = context;
		_name = name;
		_syncCounter = counter;
	}

	VoidRenderPassBuilder::VoidRenderPassBuilder(const std::string& name, IVoidRenderContext* context, core::JobBuilder& builder)
	{
		_context = context;
		_name = name;
		_syncCounter = builder.ExtractCounter();
	}


	void VoidRenderPassBuilder::AddAnonymous(const std::string& name, const std::function<void(std::shared_ptr<IVoidGraphicsCommandList>)>& func)
	{
		_jobs.emplace_back(name, func);
	}

	std::shared_ptr<core::SyncCounter> VoidRenderPassBuilder::Dispatch() const
	{
		auto list = _context->allocate_graphics_list();
		auto builder = core::JobBuilder();

		if (_syncCounter != nullptr)
			builder.DispatchWait(_syncCounter);

		for(const auto& [name, job] : _jobs)
		{
			builder.Dispatch(name, [job, list]
			{
					job(list);
			});
		}

		auto c = _context;

		builder.Dispatch("Submit list", [list, c]
			{
				c->submit_graphics_list(list);
			});

		return builder.ExtractCounter();
	}

}
