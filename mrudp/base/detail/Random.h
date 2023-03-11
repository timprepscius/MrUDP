/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include <random>
#include <limits>

namespace timprepscius {
namespace core {

struct RealRandom {
	std::random_device device;
	std::mt19937 engine;
	
	RealRandom () :
		engine(device())
	{
	}
	
	template<typename T>
	T next()
	{
		T t;
		char *begin = (char *)&t;
		char *end = begin + sizeof(T);
		for (char *i=begin; i!=end; ++i)
		{
			*i = (char)engine();
		}
		
		return t;
	}
	
	template<typename T=int>
	T nextInt (T l, T r)
	{
		auto d = r - l + 1;
		return (std::abs(next<T>()) % d) + l;
	}

	template<typename T>
	float nextReal (T min=0, T max=1)
	{
		auto d = max - min;
		
		constexpr auto dmax = 1 << std::numeric_limits<T>::digits;
		auto l = T(nextInt<uint64_t>(0, dmax));
		auto r = T(dmax);
		auto v = l/r * d + min;
		
		return v;
	}
} ;

} // namespace
} // namespace

