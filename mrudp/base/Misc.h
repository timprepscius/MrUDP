/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#ifdef MRUDP_HAS_CORE
	#include <core/containers/CircularBuffer.hpp>
	#include <core/containers/SizedVector.hpp>
	#include <core/algorithm/small_copy.hpp>
	#include <core/algorithm/mem_copy.hpp>
#else
	#include "CircularBuffer.hpp"
	#include "SizedVector.hpp"
	#include "small_copy.hpp"
	#include "mem_copy.hpp"
#endif

namespace timprepscius::mrudp {
	using namespace core;
}
