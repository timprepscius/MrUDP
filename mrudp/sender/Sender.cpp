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
	
	if (status < OPEN)
		return;

	if (!connection->canSend())
		return;

	bool sentPacket;
	
	do
	{
		sentPacket = false;
		
		if (retrier.numUnacked() < windowSize.size)
		{
			if (auto packet = dataQueue.dequeue())
			{
				packet->header.type = DATA_RELIABLE;
				sentPacket = true;
				sendImmediately(packet);
			}
		}
	}
	while(sentPacket);

	while (auto packet = unreliableDataQueue.dequeue())
	{
		packet->header.type = DATA_UNRELIABLE;
		connection->send(packet);
	}
}

bool Sender::isUninitialized ()
{
	return status == UNINITIALIZED;
}

bool Sender::empty ()
{
	return retrier.empty() && dataQueue.empty();
}

void Sender::open()
{
	xLogDebug(logOfThis(this));

	// When connections are closed, while they are negotiating their handshake
	// they may have queued data
	// --
	// So when an open is called from the handshake, even if has been officially closed
	// we process the queue.

	processSendQueue();

	if (status == UNINITIALIZED)
		status = OPEN;
}

void Sender::close()
{
	xLogDebug(logOfThis(this));

	if (status != CLOSED)
	{
		dataQueue.enqueue(CLOSE_WRITE, nullptr, 0);
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
	dataQueue.close();
	unreliableDataQueue.close();
}

void Sender::sendImmediately(const PacketPtr &packet)
{
	packet->header.id = sendIDGenerator.nextID();

	retrier.insert(packet, connection->socket->service->clock.now());
	connection->send(packet);
}

void Sender::send(const u8 *data, size_t size, Reliability reliability)
{
	if (status != CLOSED)
	{
		auto &dataQueue_ = reliability ? dataQueue : unreliableDataQueue;
		dataQueue_.enqueue(DATA, data, size);
		
		scheduleSendQueueProcessing();
	}
}

void Sender::scheduleSendQueueProcessing ()
{
	processSendQueue();
}


void Sender::onPacket(Packet &packet)
{
	if (isAck(packet.header.type))
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

	processSendQueue();
	connection->possiblyClose();
}

} // namespace
} // namespace
