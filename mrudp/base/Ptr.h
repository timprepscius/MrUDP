/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#include <memory>

template<typename T>
void on_deallocate(T *t)
{
}

namespace timprepscius {
namespace core {
namespace ptr {
namespace using_std {

template<typename T>
struct Deleter
{
	void operator()(T* p)
	{
		on_deallocate(p);
		delete p;
	}
} ;

template<typename T>
using StrongPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
using StrongThis = std::enable_shared_from_this<T>;

template<typename T, typename V>
auto strong_ptr_cast(const V &v)
{
	return std::dynamic_pointer_cast<T>(v);
}

template<typename T>
auto weak_this(T *t)
{
	return t->weak_from_this();
}

template<typename T>
auto strong_this(T *t)
{
	return std::static_pointer_cast<T>(t->weak_from_this().lock());
}

template<typename T, typename ... Args>
auto strong(Args && ...args)
{
	return std::shared_ptr<T>(new T(std::forward<Args>(args)...), Deleter<T>());
}

template<typename T>
auto strong(const WeakPtr<T> &t)
{
	return t.lock();
}

template<typename T>
auto weak(const StrongPtr<T> &t)
{
	return std::weak_ptr<T>(t);
}


template<typename T>
auto strong_thread(const T &t)
{
	return t;
}

template<typename T>
auto ptr_of(const StrongPtr<T> &t)
{
	return t.get();
}

template<typename T>
auto ref_this(T *t)
{
	return strong_this(t);
}

} // namespace
} // namespace
} // namespace
} // namespace

