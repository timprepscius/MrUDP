#pragma once

#include "debug_assert.h"
#include <vector>
#include <array>
#include <cstring>

namespace timprepscius::core {

template<typename T>
using Vector = std::vector<T>;

template<typename T, int N>
using Array = std::array<T, N>;

template<typename T>
struct CircularBuffer {
	Vector<T> buffer;
	size_t begin_ = 0, end_ = 0;
	size_t size_ = 0;
	
	size_t begin()
	{
		return begin_;
	}
	
	size_t end()
	{
		return end_;
	}
	
	void clear ()
	{
		size_ = end_ = begin_ = 0;
	}
	
	size_t size()
	{
		return size_;
	}
	
	size_t available()
	{
		return buffer.size() - size();
	}
	
	struct Segment {
		size_t to, from, size;
	} ;
	
	T *data ()
	{
		return buffer.data();
	}
	
	Array<Segment, 2> segments(size_t from_, size_t dataSize)
	{
		auto bufferSize = buffer.size();
		if (bufferSize == 0)
		{
			return {
				Segment { 0, 0, 0 },
				Segment { 0, 0, 0 }
			} ;
		}
			
		auto from = from_ % bufferSize;
		auto to_0 = from + dataSize;
		auto to_1 = 0;
		
		if (to_0 > bufferSize)
		{
			to_1 = to_0 % bufferSize;
			to_0 = to_0 - to_1;
		}
		
		auto seg_0 = (to_0 - from);
		auto seg_1 = (size_t)to_1;
	
		return {
			Segment { from, 0, seg_0 },
			Segment { 0, seg_0, seg_1 }
		} ;
	}
	
	void grow(size_t dataSize)
	{
		if (dataSize == 0)
			return;
			
		if (available() <= dataSize)
		{
			auto previousBufferSize = buffer.size();
			buffer.resize(previousBufferSize + dataSize);
			
			// if there is data at the beginning of the buffer, we need to
			// put some of it at the end
			if (end_ <= begin_)
			{
				auto previousEnd = end_;
				
				end_ = previousBufferSize;
				
				if (previousEnd > 0)
				{
					size_ -= previousEnd;
					write(buffer.data(), previousEnd);
				}
			}
		}

		size_ += dataSize;
	}
	
	void write(T *data, size_t dataSize)
	{
		auto size = size_;
		grow(dataSize);
		
		auto segments_ = segments(begin_ + size, dataSize);
		
		for (auto &segment: segments_)
		{
			if (segment.size)
			{
				debug_assert(segment.to + segment.size <= buffer.size());
				
				if (data)
					std::memcpy(buffer.data() + segment.to, data + segment.from, segment.size * sizeof(T));
				else
					std::memset(buffer.data() + segment.to, 0, segment.size * sizeof(T));
					
				end_ = segment.to + segment.size;
				end_ = end_ % buffer.size();
			}
		}
	}
	
	void peek(T *data, size_t dataSize)
	{
		if (dataSize == 0)
			return ;
			
		auto segments_ = segments(begin_, dataSize);
		for (auto &segment: segments_)
		{
			if (segment.size)
			{
				std::memcpy(data + segment.from, buffer.data() + segment.to, segment.size * sizeof(T));
			}
		}
	}
	
	void read(T *data, size_t dataSize)
	{
		if (dataSize == 0)
			return ;
			
		peek(data, dataSize);
		skip(dataSize);
	}
	
	void skip(size_t dataSize)
	{
		if (dataSize == 0)
			return ;
			
		assert(size_ >= dataSize);
		
		begin_ += dataSize;
		begin_ = begin_ % buffer.size();
		size_ -= dataSize;
	}
} ;

} // namespace
