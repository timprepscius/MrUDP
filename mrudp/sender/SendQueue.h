#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// SendQueue
//
// SendQueue houses the queue of packets yet to be sent.
//
// Notes: The internal structure of the queue is a List.  This possibly should be
// changed to std::deque.  However, in profiling, SendQueue doesn't even show up,
// and I'm not sure if std::deque maintains a buffer to fill, or whether it is
// just a list of vectors.
// --------------------------------------------------------------------------------

struct SendQueue
{
	enum Status {
		OPEN,
		CLOSED
	};

	Status status = OPEN;

	~SendQueue ();

	Mutex mutex;
	List<PacketPtr> queue;

	void enqueue(const PacketPtr &packet);
	PacketPtr dequeue();
	
	bool empty();
	void close();
};

} // namespace
} // namespace
