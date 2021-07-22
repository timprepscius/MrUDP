#pragma once

#include "../Base.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// WindowSize
//
// WindowSize calculates the maximum number of packets which should be on a
// connection with the given rtt.
//
// Notes:  when I do more performance profiling after overlapped sockets on
// linux, increasing the WindowSizeSimple::maximum might be a good thing.
// --------------------------------------------------------------------------------

struct WindowSizeConstant
{
	size_t size = 3;
	
	void onSample(float rtt) {}
} ;

struct WindowSizeSimple
{
	size_t size = 3;
	size_t minimum = 3, maximum = 128;
	
	void onSample(float rtt)
	{
		size = std::min(maximum, std::max(minimum, size_t(1 / rtt)));
	}
} ;

typedef WindowSizeSimple WindowSize;

} // namespace
} // namespace
