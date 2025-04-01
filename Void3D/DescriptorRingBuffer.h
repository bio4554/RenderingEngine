#pragma once
#include <iostream>

struct DescriptorRange
{
	uint32_t start;
	uint32_t size;
};

template<uint32_t max_size>
class DescriptorRingBuffer
{
public:
	

	DescriptorRingBuffer();
	~DescriptorRingBuffer();

	DescriptorRange allocate(uint32_t count);

private:
	std::mutex _lock;
	uint32_t _currentTop = 0;
};

template<uint32_t max_size>
inline DescriptorRingBuffer<max_size>::DescriptorRingBuffer()
{
}

template<uint32_t max_size>
inline DescriptorRingBuffer<max_size>::~DescriptorRingBuffer()
{
}

template<uint32_t max_size>
inline DescriptorRange DescriptorRingBuffer<max_size>::allocate(uint32_t count)
{
	std::unique_lock lock{ _lock };
	auto desiredIndexEnd = _currentTop + count;

	auto maximum = max_size;

	if(desiredIndexEnd > maximum)
	{
		//throw std::runtime_error("wrapped");
		//std::cout << "buffer wrapped around\n";
		_currentTop = count;
		return { 0, count };
	}

	auto startIdx = _currentTop;
	_currentTop += count;

	return { startIdx, count };
}
