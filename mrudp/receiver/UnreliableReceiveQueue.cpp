#include "UnreliableReceiveQueue.h"

namespace timprepscius {
namespace mrudp {

void UnreliableReceiveQueue::onReceive(Packet &packet)
{
	sLogDebug("mrudp::receive", logVar((char)packet.header.type) << logVar(packet.header.connection) << logVar(packet.header.id) );

	auto *begin = (Frame *)packet.data;
	auto *end = (Frame *)(packet.data + packet.dataSize);
	
	for (auto *frame = (Frame *)begin; frame < end; frame = (Frame *)(frame->data + frame->header.dataSize))
	{
		processor(*frame);
	}
}

} // namespace
} // namespace
