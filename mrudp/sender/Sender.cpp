#include "Sender.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"
#include "../Implementation.h"

namespace timprepscius {
namespace mrudp {

Sender::Sender(Connection *connection_) :
	connection(connection_),
	retrier(this)
{
}

void Sender::processSendQueue()
{
	xLogDebug(logOfThis(this));
	
	if (connection->sender.status <= Sender::SYN_SENT)
		return;

	bool sentPacket;
	
	do
	{
		sentPacket = false;
		
		if (retrier.numUnacked() < windowSize.size)
		{
			if (auto packet = sendQueue.dequeue())
			{
				sentPacket = true;
				sendImmediately(packet);
			}
		}
	}
	while(sentPacket);
}

bool Sender::isUninitialized ()
{
	return status == UNINITIALIZED;
}

bool Sender::empty ()
{
	return retrier.empty() && sendQueue.empty();
}

void Sender::open()
{
	xLogDebug(logOfThis(this));

	status = SYN_SENT;
	
	auto packet = strong<Packet>();
	packet->header.type = SYN;

	pushData(*packet, connection->localID);
	
	return sendImmediately(packet);
}

void Sender::close()
{
	xLogDebug(logOfThis(this));

	if (status != CLOSED)
	{
		auto packet = strong<Packet>();
		packet->header.type = CLOSE_WRITE;
		send(packet);

		status = CLOSED;
	}
}

void Sender::fail()
{
	xLogDebug(logOfThis(this));

	if (status != CLOSED)
	{
		status = CLOSED;
	}

	retrier.close();
	sendQueue.close();
}

void Sender::sendImmediately(const PacketPtr &packet)
{
	packet->header.id = sendIDGenerator.nextID();

	retrier.insert(packet, connection->socket->service->clock.now());
	connection->send(packet);
}

void Sender::send(const PacketPtr &packet)
{
	if (status != CLOSED)
	{
		sendQueue.enqueue(packet);
		processSendQueue();
	}
}

void Sender::onPacket(Packet &packet)
{
	if (packet.header.type == ACK || packet.header.type == SYN_ACK)
	{
		onAck(packet);
	}
	else
	if (packet.header.type == CLOSE_READ)
	{
		fail();
		
		connection->possiblyClose();
	}
}

void Sender::onAck(Packet &packet)
{
	if (status > UNINITIALIZED)
	{
		auto packetID = packet.header.id;
		
		auto now = connection->socket->service->clock.now();
		auto ackResult =
			retrier.ack(packetID, now);

		if (ackResult.contained)
		{
			rtt.onSample(ackResult.rtt);
			windowSize.onSample(rtt.duration);
			
			if (ackResult.needsRetryTimeoutRecalculation)
				retrier.recalculateRetryTimeout();
		}

		// this should have more checks
		// @TODO: think about this
		if (status == SYN_SENT)
			status = OPEN;
		
		processSendQueue();
		connection->possiblyClose();
	}
}

} // namespace
} // namespace
