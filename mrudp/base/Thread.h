/*
 * Author: Timothy Prepscius
 * Copyright: See copyright located at COPYRIGHTLOCATION
 */

#pragma once

#include "../Config.h"

#ifdef MRUDP_HAS_CORE
	#include <core/threads/Lock.hpp>
	
namespace timprepscius::mrudp {
	using namespace core;
}
#else
	#include "Mutex.h"
#endif
