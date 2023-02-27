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
	
	template<typename T>
	float nextReal ()
	{
		return T(next<int>()) / std::numeric_limits<int>::max();
	}
} ;

} // namespace
} // namespace

