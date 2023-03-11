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
	constexpr bool shouldDrop ()
	{
		return false;
	}
} ;

template<int F>
struct DropPercentage
{
	const float percentage = F/1000.0f;
	
	Random random;
	bool shouldDrop ()
	{
		auto v = random.nextReal<float>();
		return v <= percentage;
	}
} ;

#ifdef MRUDP_ENABLE_DROP
typedef DropPercentage<1000 * MRUDP_ENABLE_DROP> Drop;
#else
typedef DropNone Drop;
#endif

} // namespace
} // namespace
