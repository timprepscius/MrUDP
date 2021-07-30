#include "Receiver.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"

namespace timprepscius {
namespace mrudp {

Receiver::Receiver(Connection *connection_) :
	connection(connection_)
{
}

void Receiver::open (PacketID packetID)
{
	// When connections are closed, while they are negotiating their handshake
	// they may have queued data
	// --
	// So when an open is called from the handshake, even if has been officially closed
	// we process the queue.
	receiveQueue.expectedID = packetID;
	processReceiveQueue();

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

void Receiver::processReceived(Packet &packet)
{
	if (packet.header.type == DATA)
	{
		if (connection->receiveHandler)
			connection->receiveHandler(
				connection->userData,
				packet.data,
				packet.dataSize,
				1
			);
	}

	if (packet.header.type == DATA_UNRELIABLE)
	{
		if (connection->receiveHandler)
			connection->receiveHandler(
				connection->userData,
				packet.data,
				packet.dataSize,
				0
			);
	}
	else
	if (packet.header.type == PROBE)
	{
	
	}
	else
	if (packet.header.type == CLOSE_WRITE)
	{
		close();
		connection->possiblyClose();
	}
}

void Receiver::processReceiveQueue()
{
	receiveQueue.process(
		[this](auto &packet) {
			processReceived(packet);
		}
	);
}

inline
bool isReceivable(TypeID typeID)
{
	return
		typeID == DATA ||
		typeID == PROBE ||
		typeID == CLOSE_READ ||
		typeID == CLOSE_WRITE;
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
		if(isReceivable(type))
		{
			auto ack = strong<Packet>();
			ack->header.type = ACK;
			ack->header.id = packet.header.id;
		
			connection->send(ack);
				
			sLogDebug("mrudp::receive", logVar((char)packet.header.type) << logVar(packet.header.connection) << logVar(packet.header.id) );

			if(packet.header.id == receiveQueue.expectedID)
			{
				receiveQueue.expectedID++;

				processReceived(packet);
				processReceiveQueue();
			}
			else
			if (packet_id_greater_than(packet.header.id, receiveQueue.expectedID))
			{
				sLogDebug("mrudp::receive", logOfThis(this) << "out of order " << packet.header.id << " but greater than expected " << receiveQueue.expectedID);
				receiveQueue.enqueue(packet);
			}
			else
			{
				sLogDebug("mrudp::receive", logOfThis(this) << "DISCARD out of order " << packet.header.id << " less " << receiveQueue.expectedID);
			}
		}
		else
		if(packet.header.type == DATA_UNRELIABLE)
		{
			if(connection->receiveHandler)
			{
				connection->receiveHandler(
					connection->userData,
					packet.data,
					packet.dataSize,
					0
				);
			}
		}
	}
}

} // namespace
} // namespace
