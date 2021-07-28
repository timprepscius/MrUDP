#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// ReceiveQueue
//
// For packets which arrive out of order, the ReceiveQueue acts as an ordering
// mechansism.
//
// ReceiveQueue::process is called with a handler-function, and for each packet
// waiting and in order as expected, the given function is called.
// --------------------------------------------------------------------------------

struct ReceiveQueue
{
	PacketID expectedID;
	
	typedef OrderedMap<PacketID, Packet> Queue;
	Queue queue;
	
	// enqueues an out of order packet
	void enqueue(Packet &packet);
	
	// processes all in order and as expected packets with the given function
	void process(Function<void(Packet &)> &&f);
	
	// a helper method process the next packet
	bool processNext(const Function<void(Packet &)> &f);
};

} // namespace
} // namespace
