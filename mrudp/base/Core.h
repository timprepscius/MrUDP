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
	#include <core/assert/Assert.h>
#else
	#define sLogDebug(...)
	#define sLogDebugIf(...)
	#define xLogDebug(...)
	#define xDebugLine()
	#define xLogActivateStory(...)
	#define xLogInitialize(...)
	
	#include <cassert>
	#include "Ptr.h"
	#define debug_assert(x) assert(x)
#endif

#ifndef MRUDP_HAS_CORE
namespace timprepscius::core::ptr {
	using namespace core::ptr::using_std;
}
#endif

namespace timprepscius::mrudp {
	using namespace core::ptr;
}
