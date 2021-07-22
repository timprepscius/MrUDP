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

void SendQueue::enqueue(const PacketPtr &packet)
{
	auto lock = lock_of(mutex);
	if (status == CLOSED)
		return;

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
