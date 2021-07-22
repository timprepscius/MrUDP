/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#include <random>

namespace timprepscius {
namespace mrudp {

struct Random {
	std::random_device device;
	std::mt19937 engine;
	
	Random () :
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
	
	float nextFloat ()
	{
		return float(next<int>()) / std::numeric_limits<int>::max();
	}
} ;

} // namespace
} // namespace

