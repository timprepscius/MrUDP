#pragma once

#include "ReceiveQueue.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// UnreliableReceiveQueue
//
// ReceiveQueue::process is called with a handler-function, and for each frame in
// each packet
// --------------------------------------------------------------------------------

struct UnreliableReceiveQueue
{
	using Frame = ReceiveQueue::Frame;
	
	Function<void(Frame &)> processor;

	// process the packet immediately
	void onReceive(Packet &packet);
} ;

} // namespace
} // namespace
