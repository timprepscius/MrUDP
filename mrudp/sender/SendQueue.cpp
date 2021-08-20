#include "SendQueue.h"

namespace timprepscius {
namespace mrudp {

SendQueue::~SendQueue ()
{
	close();
}

void SendQueue::close()
{
	auto lock = lock_of(mutex);
	
	if (status != CLOSED)
	{
		status = CLOSED;
		queue.clear();
	}
}

bool SendQueue::coalesce(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode)
{
	if (mode == MRUDP_COALESCE_NONE)
		return false;

	if (queue.empty())
		return false;
		
	auto &packet = *queue.front();
	if (packet.dataSize + size + sizeof(FrameHeader) < MAX_PACKET_SIZE)
	{
		FrameHeader frameHeader {
			.type = type,
			.id = frameIDGenerator.nextID(),
			.dataSize = FrameHeader::Size(size),
		} ;
		
		pushFrame(packet, frameHeader, data);
		
		return true;
	}
		
	return false;
}

void SendQueue::enqueue(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode)
{
	auto lock = lock_of(mutex);
	if (status == CLOSED)
		return;
		
	if (coalesce(type, data, size, mode))
		return;

	auto packet = strong<Packet>();
	FrameHeader frameHeader {
		.type = type,
		.id = frameIDGenerator.nextID(),
		.dataSize = FrameHeader::Size(size),
	} ;
	
	pushFrame(*packet, frameHeader, data);

	queue.push_back(packet);
}

PacketPtr SendQueue::dequeue()
{
	auto lock = lock_of(mutex);
	if (status == CLOSED)
		return nullptr;

	if (queue.empty())
		return nullptr;
		
	auto packet = std::move(queue.front());
	queue.pop_front();
	return packet;
}

bool SendQueue::empty()
{
	auto lock = lock_of(mutex);
	return queue.empty();
}

} // namespace
} // namespace
