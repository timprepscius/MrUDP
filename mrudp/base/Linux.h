/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#include <unordered_map>

namespace std {

template<>
struct hash<__uint128_t>
{
    std::size_t operator()(__uint128_t const& v) const noexcept
    {
		return std::size_t(v);
    }
};
 
} ;

