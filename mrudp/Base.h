#pragma once

#include "Config.h"

#include <stdint.h>

#include <list>
#include <map>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <set>
#include <optional>
#include <initializer_list>
#include <queue>
#include "base/Random.h"
#include "base/Mutex.h"
#include "base/Core.h"
#include "base/Pack.h"

namespace timprepscius {
namespace mrudp {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

template<typename V>
using List = std::list<V>;

template<typename V>
using Vector = std::vector<V>;

template<typename V>
using Set = std::set<V>;

template<typename K, typename V>
using OrderedMap = std::map<K, V>;

template<typename K, typename V>
using UnorderedMap = std::unordered_map<K, V>;

template<typename V>
using Atomic = std::atomic<V>;

using Thread = std::thread;

template<typename T>
using Optional = std::optional<T>;

template<typename ... T>
using Tuple = std::tuple<T...>;

template<typename ... T>
using PriorityQueue = std::priority_queue<T...>;

template<typename T, size_t N>
using Array = std::array<T, N>;

template<typename T, size_t N>
struct StackArray
{
	size_t size_;
	Array<T, N> array;
	
	template<typename ... TS>
	StackArray(TS && ...list) :
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

typedef std::chrono::steady_clock Clock;
typedef std::chrono::time_point<Clock> Timepoint;
typedef std::chrono::milliseconds Duration;

// there must be a better way of doing the chrono stuff
// I really want duration to be std::chrono::duration<float> but then it doesn't
// add to timepoint somehow

inline
Duration toDuration(float v) {
	return Duration(int(v * 1000));
}

template<typename V>
using Function = std::function<V>;
using String = std::string;

#ifdef MRUDP_SINGLE_CHAR_TRACE

void xTraceChar_(void *self, u32, char c);
void xTraceChar_(void *self, u32, char prefix, char c);
void xTraceChar_(void *self, u32, char prefix, const std::string &c);

#define xTraceChar(...) xTraceChar_(__VA_ARGS__)

#else

#define xTraceChar(...)

#endif

} // namespace
} // namespace

