#include "ReceiveQueue.h"

namespace timprepscius {
namespace mrudp {

void ReceiveQueue::process(Function<void(Packet &)> &&f)
{
	while (processNext(f))
	{
		// intentionally left blank
	}
}

bool ReceiveQueue::processNext(const Function<void(Packet &)> &f)
{
	// does the next expected packet exist?
	auto packet_ = queue.find(expectedID);
	if (packet_ != queue.end())
	{
		auto &packet = packet_->second;
		
		// process it
		f(packet);
			
		// erase it
		queue.erase(packet_);
		
		// increase our expected ID
		++expectedID;
		
		return true;
	}
	
	return false;
}

void ReceiveQueue::enqueue(Packet &packet)
{
	auto id = packet.header.id;
	queue.emplace(id, packet);
}



} // namespace
} // namespace
