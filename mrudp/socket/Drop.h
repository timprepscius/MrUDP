#pragma once

#include "../Types.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// Drop
//
// Used for debugging, to drop packets randomly to simulate lossy connections
// --------------------------------------------------------------------------------

struct DropNone
{
	bool shouldDrop ()
	{
		return false;
	}
} ;

struct DropPercentage
{
	float percentage = 0.1f;
	
	Random random;
	bool shouldDrop ()
	{
		return random.nextFloat() < percentage;
	}
} ;

typedef DropNone Drop;

} // namespace
} // namespace
