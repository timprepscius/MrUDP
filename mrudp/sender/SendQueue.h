#pragma once

#include "../Packet.h"
#include "SendIDGenerator.h"

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

// TODO: rename SendQueue DataQueue

struct SendQueue
{
	enum Status {
		OPEN,
		CLOSED
	};

	Status status = OPEN;

	~SendQueue ();

	Mutex mutex;
	
	SendIDGenerator dataIDGenerator;
	List<PacketPtr> queue;

	bool coalesce(DataTypeID type, const u8 *data, size_t size);
	void enqueue(DataTypeID type, const u8 *data, size_t size);
	PacketPtr dequeue();
	
	bool empty();
	void close();
};

} // namespace
} // namespace
