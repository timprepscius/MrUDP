/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#ifdef MRUDP_HAS_CORE
	#include <core/ptr/Ptr.hpp>
	#include <core/system/System.h>
	#include <core/log/Log.h>
	#include <core/log/LogOf.h>
	#include <core/debug/Debug.h>
	#include <core/assert/debug_assert.h>
	#include <core/profile/PROFILE_SCOPE.h>
	#include <core/debug/Allocations.h>
	#include <core/random/Random.h>
	#include <core/containers/Pack.h>
	#include <core/containers/Array.hpp>
	#include <core/containers/InPlaceArray.hpp>
	
	#define core_only(x) x
	
namespace timprepscius::mrudp {
	using namespace core::ptr;
	
	using Random = core::RealRandom;
}

#else
	
	#include <cassert>
	#include "detail/Ptr.h"
	#include "detail/Log.h"
	#include "detail/debug_assert.h"
	#include "detail/Random.h"
	#include "detail/Pack.h"
	#include "detail/Array.h"
	#include "detail/SimpleInPlaceArray.hpp"

	#define ALLOC_RECORD(...)
	#define DEALLOC_RECORD(...)
	#define PROFILE_FUNCTION(...)
	#define core_only(x)

namespace timprepscius::mrudp {
	using namespace core::ptr;
	
	using Random = core::RealRandom;
	
	template<typename T, int N>
	using InPlaceArray = core::SimpleInPlaceArray<T, N>;
}

#endif

