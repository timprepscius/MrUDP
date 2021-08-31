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
	auto frame_ = queue.find(expectedID);
	if (frame_ != queue.end())
	{
		auto &frame = frame_->second;
		
		// process it
		processor(frame);
			
		// erase it
		queue.erase(frame_);
		
		// increase our expected ID
		++expectedID;
		
		return true;
	}
	
	return false;
}

void ReceiveQueue::onReceive(Packet &packet)
{
	sLogDebug("mrudp::receive", logVar((char)packet.header.type) << logVar(packet.header.connection) << logVar(packet.header.id) );

	auto lock = lock_of(mutex);
	
	auto *begin = (Frame *)packet.data;
	auto *end = (Frame *)(packet.data + packet.dataSize);
	
	for (auto *frame = (Frame *)begin; frame < end; frame = (Frame *)(frame->data + frame->header.dataSize))
	{
		if (frame->header.id == expectedID)
		{
			processor(*frame);
			expectedID++;

			processQueue();
		}
		else
		{
			if (id_greater_than(frame->header.id, expectedID))
			{
				sLogDebug("mrudp::receive", logOfThis(this) << "out of order " << packet.header.id << " but greater than expected " << expectedID);
				enqueue(*frame);
			}
			else
			{
				sLogDebug("mrudp::receive", logOfThis(this) << "DISCARD out of order " << packet.header.id << " less " << expectedID);
			}
		}
	}
}

void ReceiveQueue::enqueue(Frame &frame)
{
	auto id = frame.header.id;
	queue.emplace(id, frame);
}

} // namespace
} // namespace
