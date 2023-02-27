/**
 * legal header
 *
 * ============================================================================
 *
 * @author	Timothy Prepscius
 * @created 10/12/01
 */

#pragma once

#include "Array.h"

namespace timprepscius {
namespace core {

template<typename T, size_t N>
struct SimpleInPlaceArray
{
	size_t size_;
	Array<T, N> array;
	
	template<typename ... TS>
	SimpleInPlaceArray(TS && ...list) :
		size_(sizeof...(list)),
		array {{ std::forward<TS>(list)... }}
	{
	}

	auto begin()
	{
		return array.begin();
	}
	
	auto end ()
	{
		return array.begin() + size_;
	}
	
	auto begin() const
	{
		return array.begin();
	}
	
	auto end () const
	{
		return array.begin() + size_;
	}
	
	auto front() const
	{
		return array.front();
	}

	auto front()
	{
		return array.front();
	}

	auto empty() const
	{
		return array.empty();
	}
	
	auto push_back(T &&t)
	{
		debug_assert(size_ < N);
		array[size_++] = std::move(t);
	}

	size_t size() const {
		return size_;
	}
} ;

} // namespace utilities
} // namespace
