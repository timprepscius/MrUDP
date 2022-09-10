#include "UnreliableReceiveQueue.h"

namespace timprepscius {
namespace mrudp {

void UnreliableReceiveQueue::onReceive(Packet &packet)
{
	sLogDebug("mrudp::receive", logVarV((char)packet.header.type) << logVarV(packet.header.connection) << logVarV(packet.header.id) );

	auto *begin = (Frame *)packet.data;
	auto *end = (Frame *)(packet.data + packet.dataSize);
	
	for (auto *frame = (Frame *)begin; frame < end; frame = (Frame *)(frame->data + frame->header.dataSize))
	{
		processor(*frame);
	}
}

} // namespace
} // namespace
