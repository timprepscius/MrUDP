#include "Handshake.h"
#include "Connection.h"

namespace timprepscius {
namespace mrudp {

Handshake::Handshake(Connection *connection_) :
	connection(connection_)
{
}

void Handshake::initiate()
{
	auto packet = strong<Packet>();
	packet->header.type = H0;
	connection->sender.sendImmediately(packet);
	waitingFor = H1;
}

void Handshake::handlePacket(Packet &packet)
{
	auto lock = lock_of(mutex);
	
	auto readRemoteID = [this](Packet &packet) {
		ShortConnectionID remoteID_ = 0;
		if (popData(packet, remoteID_))
		{
			return remoteID_;
		}
		
		return remoteID_;
	};

	auto received = packet.header.type;
	if (received == H0)
	{
		auto ack = strong<Packet>();
		ack->header.type = H1;
		ack->header.id = packet.header.id;
	
		connection->send(ack);
		
		auto expected = H0;
		waitingFor.compare_exchange_strong(expected, H2);
	}
	else
	if (received == H1)
	{
		auto expected = H1;
		if (waitingFor.compare_exchange_strong(expected, H3))
		{
			auto packet = strong<Packet>();
			packet->header.type = H2;
			pushData(*packet, connection->localID);
			connection->sender.sendImmediately(packet);
		}
	}
	else
	if (received == H2)
	{
		auto remoteID = readRemoteID(packet);
	
		auto ack = strong<Packet>();
		ack->header.type = H3;
		ack->header.id = packet.header.id;
		pushData(*ack, connection->localID);
	
		connection->send(ack);
		
		auto expected = H2;
		if (waitingFor.compare_exchange_strong(expected, HANDSHAKE_COMPLETE))
		{
			connection->remoteID = remoteID;
			firstNonHandshakePacketID = packet.header.id + 1;
			onHandshakeComplete();
		}
	}
	else
	if (received == H3)
	{
		auto remoteID = readRemoteID(packet);

		auto expected = H3;
		if (waitingFor.compare_exchange_strong(expected, HANDSHAKE_COMPLETE))
		{
			connection->remoteID = remoteID;
			onHandshakeComplete();
		}
	}
	else
	{
		// this should never happen
		debug_assert(false);
	}
}

void Handshake::onPacket(Packet &packet)
{
	if (!isHandshake(packet.header.type))
		return;

	handlePacket(packet);
}

void Handshake::onHandshakeComplete()
{
	connection->receiver.open(firstNonHandshakePacketID);
	connection->sender.open();
}

} // namespace
} // namespace
