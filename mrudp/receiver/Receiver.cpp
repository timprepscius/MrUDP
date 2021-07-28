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
	status = OPEN;
	receiveQueue.expectedID = packetID;
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

void Receiver::onPacket(Packet &packet)
{
	if (packet.header.type == CLOSE_READ)
	{
		xDebugLine();
	}

	if(status == Receiver::UNINITIALIZED)
	{
		if(packet.header.type == SYN)
		{
			open(packet.header.id);
			
			auto ack = strong<Packet>();
			ack->header.type = SYN_ACK;
			ack->header.id = receiveQueue.expectedID++;
			pushData(*ack, connection->localID);
			
			connection->send(ack);
		}
		else
		{
			// No connections exist and we got a non syn packet
			// We ignore this
		}
	}
	else
	{
		if(packet.header.type == DATA || packet.header.type == PROBE || packet.header.type == CLOSE_WRITE || packet.header.type == CLOSE_READ)
		{
			auto ack = strong<Packet>();
			ack->header.type = ACK;
			ack->header.id = packet.header.id;
		
			connection->send(ack);
				
			sLogDebug("mrudp::receive", logVar((char)packet.header.type) << logVar(packet.header.connection) << logVar(packet.header.id) <<  logVar(receiveQueue.expectedID));

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
