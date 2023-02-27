/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "small_copy.hpp"

namespace timprepscius {
namespace core {

inline
void mem_copy(char *d, const char *s, size_t size)
{
	if (!maybe_small_copy(d, s, size))
		memcpy(d, s, size);
}

} // namespace
} // namespace
