#include "Receiver.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"

namespace timprepscius {
namespace mrudp {

Receiver::Receiver(Connection *connection_) :
	connection(connection_)
{
	receiveQueue.processor =
		[this](auto &packet) {
			processReceived(packet, RELIABLE);
		};

	unreliableReceiveQueue.processor =
		[this](auto &packet) {
			processReceived(packet, UNRELIABLE);
		};
}

void Receiver::open (PacketID packetID)
{
	// When connections are closed, while they are negotiating their handshake
	// they may have queued data
	// --
	// So when an open is called from the handshake, even if has been officially closed
	// we process the queue.
//	receiveQueue.expectedID = 0;
	receiveQueue.processQueue();

	if (status == UNINITIALIZED)
		status = OPEN;
}

void Receiver::close ()
{
	if (status < CLOSED)
	{
		if (connection->canSend())
		{
			auto packet = strong<Packet>();
			packet->header.type = CLOSE_READ;
		
			connection->sender.sendImmediately(packet);
		}
		
		status = CLOSED;
	}
}

void Receiver::fail ()
{
	if (status < CLOSED)
	{
		status = CLOSED;
	}
}

void Receiver::processReceived(ReceiveQueue::Datum &datum, Reliability reliability)
{
	if (datum.header.type == DATA)
	{
		if (connection->receiveHandler)
			connection->receiveHandler(
				connection->userData,
				datum.data,
				datum.header.dataSize,
				reliability
			);
	}
	else
	if (datum.header.type == CLOSE_WRITE)
	{
		if (reliability == RELIABLE)
		{
			close();
			connection->possiblyClose();
		}
	}
}

inline
bool requiresAck(TypeID typeID)
{
	return
		typeID == DATA_RELIABLE ||
		typeID == PROBE ||
		typeID == CLOSE_READ;
}

void Receiver::onPacket(Packet &packet)
{
	if (packet.header.type == CLOSE_READ)
	{
		xDebugLine();
	}

	if (status == OPEN)
	{
		auto type = packet.header.type;
		if(requiresAck(type))
		{
			auto ack = strong<Packet>();
			ack->header.type = ACK;
			ack->header.id = packet.header.id;
			connection->send(ack);
		}

		if (packet.header.type == DATA_RELIABLE)
			receiveQueue.process(packet);
		else
		if (packet.header.type == DATA_UNRELIABLE)
			unreliableReceiveQueue.process(packet);
	}
}

} // namespace
} // namespace
