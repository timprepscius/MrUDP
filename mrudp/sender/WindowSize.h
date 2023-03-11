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

struct WindowSizeMinMaxLinear
{
	size_t size = 3;
	static constexpr size_t minimum = 3, maximum = 256;
	static constexpr size_t maximumLength = maximum - minimum;
	
	static constexpr float maximumRtt = 1.0;
	
	void onSample(float rtt)
	{
		rtt = std::min(rtt, maximumRtt);
		auto drtt = maximumRtt - rtt;
		size = drtt / maximumRtt * maximumLength + minimum;
	}
} ;

struct WindowSizeSimple
{
	size_t size = 3;
	size_t minimum = 3, maximum = 1024;
	
	void onSample(float rtt)
	{
		size = std::min(maximum, std::max(minimum, size_t(2 / rtt)));
	}
} ;

typedef WindowSizeSimple WindowSize;

} // namespace
} // namespace
