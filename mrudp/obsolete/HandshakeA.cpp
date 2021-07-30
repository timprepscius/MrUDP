#include "HandshakeA.h"
#include "Connection.h"

namespace timprepscius {
namespace mrudp {

HandshakeA::HandshakeA(Connection *connection_) :
	connection(connection_)
{
}

void HandshakeA::sendPacket (TypeID type, PacketID packetID)
{
	auto packet = strong<Packet>();
	packet->header.type = type;

	// this really needs to go somewhere else
	// for handshake packets past H1, there is encryption
	// so we send the local ID
	if (type >= H2)
		pushData(*packet, connection->localID);
	
	// if we are sending an ack
	if (isAck(type))
	{
		// then make sure it acks the correct packet
		packet->header.id = packetID;
		
		// and send it directly
		connection->send(packet);
	}
	// if we are not sending an ack
	else
	{
		// send it through the reliable mechanism
		connection->sender.sendImmediately(packet);
	}
	
	if (step == type)
	{
		step = TypeID(step + 1);
	}
}

void HandshakeA::initiate()
{
	sendPacket (step, 0);
}

void HandshakeA::receivePacket(Packet &packet)
{
	auto type = packet.header.type;

	if (type >= H2)
	{
		ShortConnectionID remoteID_;
		if (popData(packet, remoteID_))
		{
			if (step == type)
			{
				connection->remoteID = remoteID_;
			}
		}
	}

	bool isAck = (type == H1 || type == H3);
	bool requiresAck = !isAck;

	if (step == type)
	{
		step = TypeID(step + 1);
		
		if (step < HANDSHAKE_COMPLETE)
		{
			sendPacket(step, packet.header.id);
		}
		
		if (step == HANDSHAKE_COMPLETE)
		{
			if (!isAck)
			{
				firstNonHandshakePacketID = packet.header.id + 1;
			}
			onHandshakeComplete();
		}
	}
	else
	{
		if (requiresAck)
		{
			auto ackType = TypeID(type+1);
			sendPacket(ackType, packet.header.id);
		}
	}
}

void HandshakeA::onPacket(Packet &packet)
{
	if (!isHandshake(packet.header.type))
		return;

	receivePacket(packet);
}

void HandshakeA::onHandshakeComplete()
{
	connection->receiver.open(firstNonHandshakePacketID);
	connection->sender.open();
}

} // namespace
} // namespace
