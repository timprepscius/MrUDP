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
	schedules[0].name = "send-0";
	schedules[1].name = "send-1";
}

void Sender::processReliableDataQueue()
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
				sendReliably(packet);
			}
		}
	}
	while(sentPacket);
}

void Sender::processUnreliableDataQueue()
{
	xLogDebug(logOfThis(this));
	
	if (status >= OPEN && connection->canSend())
	{
		while (auto packet = unreliableDataQueue.dequeue())
		{
			packet->header.type = DATA_UNRELIABLE;
			connection->send(packet);
		}
	}
	else
	{
		unreliableDataQueue.clear();
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

	scheduleDataQueueProcessing(RELIABLE);
	scheduleDataQueueProcessing(UNRELIABLE);

	if (status == UNINITIALIZED)
		status = OPEN;
}

void Sender::close()
{
	xLogDebug(logOfThis(this));

	if (status != CLOSED)
	{
		dataQueue.enqueue(
			CLOSE_WRITE,
			nullptr, 0,
			MRUDP_COALESCE_PACKET
		);
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

void Sender::sendReliably(const PacketPtr &packet)
{
	packet->header.id = packetIDGenerator.nextID();

	retrier.insert(packet, connection->socket->service->clock.now());
	connection->send(packet);
}

ErrorCode Sender::send(const u8 *data, size_t size, Reliability reliability)
{
	if (status != CLOSED)
	{
		auto mode = reliability ?
			(SendQueue::CoalesceMode)connection->options.coalesce_reliable.mode :
			(SendQueue::CoalesceMode)connection->options.coalesce_unreliable.mode;

		auto &dataQueue_ = reliability ? dataQueue : unreliableDataQueue;

		if (size > MAX_PACKET_DATA_SIZE && mode != MRUDP_COALESCE_STREAM)
			return ERROR_PACKET_SIZE_TOO_LARGE;

		dataQueue_.enqueue(
			DATA,
			data, size,
			mode
		);
		
		scheduleDataQueueProcessing(reliability);
		
		return OK;
	}
	
	return ERROR_CONNECTION_CLOSED;
}

void Sender::scheduleDataQueueProcessing (Reliability reliability)
{
	auto &options = reliability ?
		connection->options.coalesce_reliable :
		connection->options.coalesce_unreliable;

	if (options.mode == MRUDP_COALESCE_NONE)
		return processDataQueue(reliability);

	debug_assert((size_t)reliability >= 0 && (size_t)reliability < 2);
	auto &schedule = schedules[(size_t)reliability];
	if (schedule.waiting)
		return ;
		
	schedule.waiting = true;

	auto sendQueueProcessingDelay = options.delay_ms;
	auto now = connection->socket->service->clock.now();
	auto then = now + Duration(sendQueueProcessingDelay);
	
	connection->imp->setTimeout(
		schedule.name, then,
		[this, &schedule, reliability]() {
			schedule.waiting = false;
			processDataQueue(reliability);
		}
	);
}

void Sender::processDataQueue(Reliability reliability)
{
	if (reliability)
	{
		processReliableDataQueue();
	}
	else
	{
		processUnreliableDataQueue();
	}
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
	auto ackResult = retrier.ack(packetID, now);

	if (ackResult.contained)
	{
		rtt.onSample(ackResult.rtt);
		windowSize.onSample(rtt.duration);
		
		if (ackResult.needsRetryTimeoutRecalculation)
			retrier.recalculateRetryTimeout();
	}

	scheduleDataQueueProcessing(RELIABLE);
	connection->possiblyClose();
}

} // namespace
} // namespace
