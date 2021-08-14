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
	auto datum_ = queue.find(expectedID);
	if (datum_ != queue.end())
	{
		auto &datum = datum_->second;
		
		// process it
		processor(datum);
			
		// erase it
		queue.erase(datum_);
		
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
	
	auto *begin = (Datum *)packet.data;
	auto *end = (Datum *)(packet.data + packet.dataSize);
	
	for (auto *datum = (Datum *)begin; datum < end; datum = (Datum *)(datum->data + datum->header.dataSize))
	{
		if (datum->header.id == expectedID)
		{
			processor(*datum);
			expectedID++;

			processQueue();
		}
		else
		{
			if (packet_id_greater_than(datum->header.id, expectedID))
			{
				sLogDebug("mrudp::receive", logOfThis(this) << "out of order " << packet.header.id << " but greater than expected " << expectedID);
				enqueue(*datum);
			}
			else
			{
				sLogDebug("mrudp::receive", logOfThis(this) << "DISCARD out of order " << packet.header.id << " less " << expectedID);
			}
		}
	}
}

void ReceiveQueue::enqueue(Datum &datum)
{
	auto id = datum.header.id;
	queue.emplace(id, datum);
}

} // namespace
} // namespace
