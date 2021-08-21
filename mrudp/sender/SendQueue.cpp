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

bool SendQueue::coalescePacket(FrameTypeID type, const u8 *data, size_t size)
{
	if (queue.empty())
		return false;
		
	auto &packet = *queue.back();
	if (packet.dataSize + size + sizeof(FrameHeader) < MAX_PACKET_SIZE)
	{
		FrameHeader frameHeader {
			.id = frameIDGenerator.nextID(),
			.type = type,
			.dataSize = FrameHeader::Size(size),
		} ;
		
		pushFrame(packet, frameHeader, data);
		
		return true;
	}
		
	return false;
}

bool SendQueue::coalesceStream(FrameTypeID type, const u8 *data, size_t size)
{
	if (queue.empty())
		return false;

	while (size > 0)
	{
		auto &packet = *queue.back();
		auto availableWriteSize = MAX_PACKET_SIZE - int(packet.dataSize + sizeof(FrameHeader));
		if (availableWriteSize > 0)
		{
			auto writeSize = std::min((size_t)availableWriteSize, size);
		
			FrameHeader frameHeader {
				.id = frameIDGenerator.nextID(),
				.type = type,
				.dataSize = FrameHeader::Size(writeSize),
			} ;
			
			pushFrame(packet, frameHeader, data);
			
			size -= writeSize;
		}
		else
		{
			auto packet = strong<Packet>();
			queue.push_back(packet);
		}
	}
		
	return true;
}

bool SendQueue::coalesce(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode)
{
	if (mode == MRUDP_COALESCE_NONE)
		return false;
		
	if (mode == MRUDP_COALESCE_PACKET)
		return coalescePacket(type, data, size);
		
	if (mode == MRUDP_COALESCE_STREAM)
		return coalesceStream(type, data, size);
		
	debug_assert(false);
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
		.id = frameIDGenerator.nextID(),
		.type = type,
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

void SendQueue::clear ()
{
	auto lock = lock_of(mutex);
	queue.clear();
}

} // namespace
} // namespace
