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
	receiveQueue.processQueue();

	if (status == UNINITIALIZED)
		status = OPEN;
}

void Receiver::close ()
{
	if (status < CLOSED)
	{
		// TODO:
		// this should send even if we haven't finished the handshake
		// as long as we have finished the first packet handshake and have the RSA keys
		// this will help connections time out faster.
		// maybe it should be canSend & also finishedHandshake in the sender enqueue
		
		if (connection->canSend())
		{
			auto packet = strong<Packet>();
			packet->header.type = CLOSE_READ;
		
			connection->sender.sendReliably(packet);
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

void Receiver::processReceived(ReceiveQueue::Frame &frame, Reliability reliability)
{
	if (frame.header.type == DATA)
	{
		connection->receive(frame.data, frame.header.dataSize, reliability);
	}
	else
	if (frame.header.type == CLOSE_WRITE)
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

void Receiver::onReceive(Packet &packet)
{
	if (status != OPEN)
		return;
		
	// TODO:
	// this needs to keep track of packet ids which arrive out of order, maintain the last good ID
	// and only accept packets after it
		
	auto type = packet.header.type;
	if(requiresAck(type))
	{
		auto ack = strong<Packet>();
		ack->header.type = ACK;
		ack->header.id = packet.header.id;
		connection->send(ack);
	}

	if (packet.header.type == DATA_RELIABLE)
	{
		receiveQueue.process(packet);
	}
	else
	if (packet.header.type == DATA_UNRELIABLE)
	{
		unreliableReceiveQueue.process(packet);
	}
}

} // namespace
} // namespace
