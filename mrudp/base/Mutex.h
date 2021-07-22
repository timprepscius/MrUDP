/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace timprepscius {
namespace mrudp {

typedef std::recursive_mutex RecursiveMutex;
typedef std::mutex Mutex;
typedef std::condition_variable Event;

template<typename T>
std::lock_guard<T> lock_of(T &m)
{
	return std::lock_guard<T>(m);
}

} // namespace
} // namespace

