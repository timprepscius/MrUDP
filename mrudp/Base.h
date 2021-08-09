#pragma once

#include "Config.h"

#include <list>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <set>
#include "base/Random.h"
#include "base/Mutex.h"
#include "base/Core.h"

#ifdef SYS_LINUX
	#include "base/Linux.h"
#endif

namespace timprepscius {
namespace mrudp {

using u8 = uint8_t;
using u16 = uint16_t;

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

typedef std::chrono::system_clock Clock;
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

void xTraceChar_(void *self, uint32_t, char c);
void xTraceChar_(void *self, uint32_t, char prefix, char c);
void xTraceChar_(void *self, uint32_t, char prefix, const std::string &c);

#define xTraceChar(...) xTraceChar_(__VA_ARGS__)

#else

#define xTraceChar(...)

#endif

} // namespace
} // namespace

