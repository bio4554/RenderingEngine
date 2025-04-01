#include "pch.h"
#include "JobSystem.h"
#include "tracy/Tracy.hpp"

namespace star::core {
	JobBuilder::JobBuilder()
	{
		_accumulator = std::make_shared<SyncCounter>();
		_zeroCounter = std::make_shared<SyncCounter>();
	}

	void JobBuilder::DispatchNow(std::string name, std::function<void()> job)
	{
		Guard();
		auto accumulator = _accumulator;
		accumulator->Increment();
		GTaskSystem->async_([job, name, accumulator]
			{
				job();
				accumulator->Decrement();
			}, _zeroCounter);
	}

	void JobBuilder::DispatchWait()
	{
		Guard();
		ExplicitFence();
		/*++_syncCounter;
		auto f = GTaskSystem->async_([]
		{
			
		}, fence);
		_currentFrontJobFence = f;
		_lastJob = f;*/
	}

	void JobBuilder::DispatchWait(std::shared_ptr<SyncCounter> counter)
	{
		Guard();
		_zeroCounter = counter;
		_accumulator = std::make_shared<SyncCounter>();
	}

	void JobBuilder::Dispatch(std::string name, std::function<void()> job)
	{
		Guard();
		auto accumulator = _accumulator;
		accumulator->Increment();
		GTaskSystem->async_([job, name, accumulator]
		{
				job();
				accumulator->Decrement();
		}, _zeroCounter);
		ExplicitFence();
	}

	void JobBuilder::WaitAll()
	{
		Guard();
		// todo hack
		while(!_zeroCounter->IsZero()) {}
	}

	std::shared_ptr<SyncCounter> JobBuilder::ExtractCounter()
	{
		Guard();
		_counterExtracted = true;
		return _zeroCounter;
	}

	void JobBuilder::ExplicitFence()
	{
		_zeroCounter = _accumulator;
		_accumulator = std::make_shared<SyncCounter>();
	}

}
