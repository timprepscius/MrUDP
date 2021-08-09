#include "ReceiveQueue.h"

namespace timprepscius {
namespace mrudp {

void ReceiveQueue::processQueue()
{
	while (processNext())
	{
		// intentionally left blank
	}
}

bool ReceiveQueue::processNext()
{
	// does the next expected packet exist?
	auto packet_ = queue.find(expectedID);
	if (packet_ != queue.end())
	{
		auto &packet = packet_->second;
		
		// process it
		processor(packet);
			
		// erase it
		queue.erase(packet_);
		
		// increase our expected ID
		++expectedID;
		
		return true;
	}
	
	return false;
}

void ReceiveQueue::process(Packet &packet)
{
	sLogDebug("mrudp::receive", logVar((char)packet.header.type) << logVar(packet.header.connection) << logVar(packet.header.id) );

	auto lock = lock_of(mutex);
	
	if(packet.header.id == expectedID)
	{
		processor(packet);
		
		expectedID++;
		processQueue();
	}
	else
	if (packet_id_greater_than(packet.header.id, expectedID))
	{
		sLogDebug("mrudp::receive", logOfThis(this) << "out of order " << packet.header.id << " but greater than expected " << expectedID);
		enqueue(packet);
	}
	else
	{
		sLogDebug("mrudp::receive", logOfThis(this) << "DISCARD out of order " << packet.header.id << " less " << expectedID);
	}

}

void ReceiveQueue::enqueue(Packet &packet)
{
	auto id = packet.header.id;
	queue.emplace(id, packet);
}



} // namespace
} // namespace
