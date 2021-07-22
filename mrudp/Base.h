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
#include "base/Random.h"
#include "base/Mutex.h"
#include "base/Core.h"

#ifdef SYS_LINUX
	#include "base/Linux.h"
#endif

namespace timprepscius {
namespace mrudp {

template<typename V>
using List = std::list<V>;

template<typename V>
using Vector = std::vector<V>;

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


void trace_char_(char c);
void trace_char_(const String &c);

inline
void trace_char(char c)
{
#ifdef MRUDP_SINGLE_CHAR_TRACE
	trace_char_(c);
#endif
}

inline
void trace_char(const String &c)
{
#ifdef MRUDP_SINGLE_CHAR_TRACE
	trace_char_(c);
#endif
}

} // namespace
} // namespace

