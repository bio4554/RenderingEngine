#pragma once
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <client/TracyArmCpuTable.hpp>
#include <common/TracySystem.hpp>
#include "SyncCounter.h"

namespace star::core
{
	class NotificationQueue
	{
	public:
		bool TryPop(std::pair<std::shared_ptr<SyncCounter>, std::function<void()>>& func)
		{
			std::unique_lock<std::mutex> lock{_mutex, std::try_to_lock};
			if (!lock || _q.empty()) return false;
			func = std::move(_q.front());
			_q.pop_front();
			return true;
		}

		template <typename F>
		bool TryPush(F&& f, const std::shared_ptr<SyncCounter>& fence)
		{
			{
				std::unique_lock<std::mutex> lock{_mutex, std::try_to_lock};
				if (!lock) return false;
				_q.emplace_back(std::pair(fence, std::forward<F>(f)));
			}

			_ready.notify_one();
			return true;
		}

		bool Pop(std::pair<std::shared_ptr<SyncCounter>, std::function<void()>>& func)
		{
			std::unique_lock<std::mutex> lock{_mutex};
			while (_q.empty() && !_done) _ready.wait(lock);
			if (_q.empty()) return false;
			func = std::move(_q.front());
			_q.pop_front();
			return true;
		}

		template <typename F>
		void Push(F&& f, const std::shared_ptr<SyncCounter>& fence)
		{
			{
				std::unique_lock<std::mutex> lock{_mutex};
				_q.emplace_back(std::pair(fence, std::forward<F>(f)));
			}
			_ready.notify_one();
		}

		void Done()
		{
			{
				std::unique_lock<std::mutex> lock{_mutex};
				_done = true;
			}
			_ready.notify_all();
		}

	private:
		std::deque<std::pair<std::shared_ptr<SyncCounter>, std::function<void()>>> _q;
		bool _done{false};
		std::mutex _mutex;
		std::condition_variable _ready;
	};

	class TaskSystem
	{
	public:
		TaskSystem()
		{
			for (uint32_t i = 0; i != _count; i++)
			{
				_threads.emplace_back([&, i] { Run(i); });
			}
		}

		~TaskSystem()
		{
			for (auto& q : _q)
			{
				q.Done();
			}

			for (auto& t : _threads)
			{
				t.join();
			}
		}

		template <typename F>
		void async_(F&& f, std::shared_ptr<SyncCounter> dep)
		{
			auto i = _index++;

			for (uint32_t n = 0; n != _count * _k; ++n)
			{
				if (_q[(i + n) % _count].TryPush(std::forward<F>(f), dep)) return;
			}

			_q[i % _count].Push(std::forward<F>(f), dep);
		}

	private:
#ifdef _DEBUG
		const uint32_t _count{1};
#else
		const uint32_t _count{ std::thread::hardware_concurrency() };
#endif
		std::vector<std::thread> _threads;
		std::vector<NotificationQueue> _q{_count};
		std::atomic<uint32_t> _index{0};
		/*std::atomic<uint64_t> _fence_counter{ 0 };
		std::atomic<uint64_t> _fence{ 0 };*/
		static constexpr uint32_t _k = 5;

		void PushAny(const std::function<void()>& f, const std::shared_ptr<SyncCounter>& fenceVal)
		{
			const auto i = _index++;

			for (uint32_t n = 0; n != _count * _k; ++n)
			{
				if (_q[(i + n) % _count].TryPush(f, fenceVal)) return;
			}

			_q[i % _count].Push(f, fenceVal);
		}

		void Run(const uint32_t i)
		{
			auto threadName = "Worker Thread " + std::to_string(i);
			tracy::SetThreadName(threadName.c_str());
			while (true)
			{
				bool gotFunc = false;
				std::pair<std::shared_ptr<SyncCounter>, std::function<void()>> f;

				for (uint32_t n = 0; n != _count; ++n)
				{
					if (_q[(i + n) % _count].TryPop(f))
					{
						gotFunc = true;
						break;
					}
				}

				if (!gotFunc && !_q[i].Pop(f)) break;

				if (!f.first->IsZero())
				{
					PushAny(f.second, f.first);
				}
				else
				{
					f.second();
				}
			}
		}
	};

	extern TaskSystem* GTaskSystem;
}
