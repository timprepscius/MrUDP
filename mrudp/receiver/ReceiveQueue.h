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
// ReceiveQueue::onReceive is called for each incoming packet, which in turn
// calls the processor function for each frame in each packet in order;
// --------------------------------------------------------------------------------

struct ReceiveQueue
{
	struct Frame {
		typedef char Data[MAX_PACKET_SIZE];

		FrameHeader header;
		Data data;
	} __attribute__ ((packed));

	FrameID expectedID = 0;
	Function<void(Frame &)> processor;
	
	Mutex mutex;
	typedef OrderedMap<FrameID, Frame> Queue;
	Queue queue;
	
	// enqueues an out of order packet
	void enqueue(Frame &packet);
	
	// either process the packet immediately, enqueue it or
	// discard it
	void onReceive(Packet &packet);
	
	// processes all in order and as expected packets with the given function
	void processQueue ();
	
	// a helper method process the next packet
	bool processNext();
};

} // namespace
} // namespace
