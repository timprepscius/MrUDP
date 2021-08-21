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
	using CoalesceMode = mrudp_coalesce_mode_t;

	~SendQueue ();

	Mutex mutex;
	
	SendIDGenerator frameIDGenerator;
	List<PacketPtr> queue;

	bool coalescePacket(FrameTypeID type, const u8 *data, size_t size);
	bool coalesceStream(FrameTypeID type, const u8 *data, size_t size);

	bool coalesce(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode);
	void enqueue(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode);
	PacketPtr dequeue();
	
	bool empty();
	void clear();
	void close();
};

} // namespace
} // namespace
