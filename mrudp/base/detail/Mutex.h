/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <shared_mutex>

namespace timprepscius {
namespace mrudp {

typedef std::shared_mutex SharedMutex;
typedef std::recursive_mutex RecursiveMutex;
typedef std::mutex Mutex;
typedef std::condition_variable Event;

template<typename T>
std::lock_guard<T> lock_of(T &m)
{
	return std::lock_guard<T>(m);
}


template<typename SharedMutex>
class shared_lock_guard
{
private:
    SharedMutex& m;

public:
    typedef SharedMutex mutex_type;
    explicit shared_lock_guard(SharedMutex& m_):
        m(m_)
    {
        m.lock_shared();
    }
    shared_lock_guard(SharedMutex& m_,std::adopt_lock_t):
        m(m_)
    {}
    ~shared_lock_guard()
    {
        m.unlock_shared();
    }
};

inline
shared_lock_guard<SharedMutex> shared_lock_of(SharedMutex &m_)
{
	return shared_lock_guard<SharedMutex>(m_);
}

} // namespace
} // namespace

