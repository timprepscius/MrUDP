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

bool SendQueue::coalesce(DataTypeID type, const u8 *data, size_t size)
{
	if (queue.empty())
		return false;
		
	auto &packet = *queue.front();
	if (packet.dataSize + size + sizeof(DataHeader) < MAX_PACKET_SIZE)
	{
		DataHeader dataHeader {
			.type = type,
			.id = dataIDGenerator.nextID(),
			.dataSize = DataHeader::Size(size),
		} ;
		
		pushData(packet, dataHeader, data);
		
		return true;
	}
		
	return false;
}

void SendQueue::enqueue(DataTypeID type, const u8 *data, size_t size)
{
	auto lock = lock_of(mutex);
	if (status == CLOSED)
		return;
		
	if (coalesce(type, data, size))
		return;

	auto packet = strong<Packet>();
	DataHeader dataHeader {
		.type = type,
		.id = dataIDGenerator.nextID(),
		.dataSize = DataHeader::Size(size),
	} ;
	
	pushData(*packet, dataHeader, data);

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
