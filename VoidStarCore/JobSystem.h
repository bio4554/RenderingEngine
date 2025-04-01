#pragma once

#include "SyncCounter.h"
#include "TaskSystem.h"

namespace star::core
{
	class JobBuilder
	{
	public:
		JobBuilder();
		void DispatchNow(std::string name, std::function<void()> job);
		void Dispatch(std::string name, std::function<void()> job);
		void DispatchWait();
		void DispatchWait(std::shared_ptr<SyncCounter> counter);
		void WaitAll();
		std::shared_ptr<SyncCounter> ExtractCounter();
	private:
		void ExplicitFence();
		void Guard() const
		{
			if (_counterExtracted)
				throw std::runtime_error("Counter already extracted!");
		}

		bool _counterExtracted = false;
		std::shared_ptr<SyncCounter> _accumulator;
		std::shared_ptr<SyncCounter> _zeroCounter;
	};
}