#include "Sender.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"
#include "../Implementation.h"

namespace timprepscius {
namespace mrudp {

Sender::Sender(Connection *connection_) :
	connection(connection_),
	retrier(this),
	dataQueue(&connection->options.coalesce_reliable),
	unreliableDataQueue(&connection->options.coalesce_unreliable)
{
	connection->socket->service->scheduler->allocate(
		schedules[0].timeout,
		[this]() {
			processSchedule(UNRELIABLE);
		}
	);
	
	connection->socket->service->scheduler->allocate(
		schedules[1].timeout,
		[this]() {
			processSchedule(RELIABLE);
		}
	);
}

Timepoint Sender::now()
{
	return connection->socket->service->clock.now();
}

void Sender::processSchedule(Reliability mode)
{
	auto &schedule = schedules[(size_t)mode];

//	auto time = now();
//	Timepoint when;
	
	{
		auto lock = lock_of(schedule.mutex);
		if (schedule.running)
			return;

		debug_assert(!schedule.running);
//		when = *schedule.when;
		schedule.when.reset();
		schedule.running = true;
	}

//	auto time1 = now();
//	auto delay0 = std::chrono::duration_cast<Duration>(time1 - time).count();
//	auto delay = std::chrono::duration_cast<Duration>(time - when).count();
//
//	sLogReleaseIf(delay > 20, "debug", logOfThis(this) << logVar(mode) << logVar(delay) << logVar(delay0));


	processDataQueue(mode);

//	auto after = std::chrono::duration_cast<Duration>(now() - when).count();
//	sLogReleaseIf(after > 20, "debug", logOfThis(this) << logVar(mode)  << logVar(after));

	{
		auto lock = lock_of(schedule.mutex);
		debug_assert(schedule.running);
		schedule.running = false;
		if (schedule.when)
		{
//			auto nexter = std::chrono::duration_cast<Duration>(*schedule.when - now()).count();
//			sLogRelease("debug", logOfThis(this) << logVar(mode) << logVar(nexter));
//
//			if (nexter < 0)
//			{
//				xDebugLine();
//			}

			schedule.timeout.schedule(*schedule.when);
		}
	}
}

void Sender::processReliableDataQueue()
{
	xLogDebug(logOfThis(this));
	
	if (!isReadyToSend())
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
	
	if (isReadyToSend())
	{
		queueDelayedAcks();
	
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

bool Sender::isReadyToSend ()
{
	return status >= OPEN && connection->canSend();
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

	if (status == UNINITIALIZED)
		status = OPEN;

	scheduleDataQueueProcessing(RELIABLE);
	scheduleDataQueueProcessing(UNRELIABLE);
}

void Sender::close()
{
	xLogDebug(logOfThis(this));

	if (status != CLOSED)
	{
		enqueue(
			CLOSE_WRITE,
			nullptr, 0,
			RELIABLE,
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

void Sender::sendReliablyMultipath(MultiPacketPath &multipath, bool priority)
{
	auto id = packetIDGenerator.nextID();
	
	for (auto &path: multipath)
		path.packet->header.id = id;

	retrier.insert(multipath, connection->socket->service->clock.now(), priority);

	for (auto &path: multipath)
		connection->send(path.packet, path.address ? &*path.address : nullptr);
}

void Sender::sendReliably(const PacketPtr &packet, const Address *address)
{
	MultiPacketPath multipath = { PacketPath { packet }};
	if (address)
		multipath.front().address = *address;
		
	sendReliablyMultipath(multipath, false);
}

void Sender::enqueue(FrameTypeID typeID, const u8 *data, size_t size, Reliability reliability, SendQueue::CoalesceMode mode)
{
	auto &dataQueue_ = reliability ? dataQueue : unreliableDataQueue;

	dataQueue_.enqueue(
		typeID,
		data, size,
		mode
	);
	
	if (isReadyToSend())
		scheduleDataQueueProcessing(reliability);
}

ErrorCode Sender::send(const u8 *data, size_t size, Reliability reliability)
{
	if (status != CLOSED)
	{
		auto mode = reliability ?
			(SendQueue::CoalesceMode)connection->options.coalesce_reliable.mode :
			(SendQueue::CoalesceMode)connection->options.coalesce_unreliable.mode;

		if (size > MAX_PACKET_DATA_SIZE &&
			mode != MRUDP_COALESCE_STREAM &&
			mode != MRUDP_COALESCE_STREAM_COMPRESSED
		)
			return ERROR_PACKET_SIZE_TOO_LARGE;

		enqueue(DATA, data, size, reliability, mode);
		
		return OK;
	}
	
	return ERROR_CONNECTION_CLOSED;
}

void Sender::scheduleDataQueueProcessing (Reliability reliability, bool immediate)
{
	auto &options = reliability ?
		connection->options.coalesce_reliable :
		connection->options.coalesce_unreliable;

	if (options.mode == MRUDP_COALESCE_NONE)
		return processDataQueue(reliability);

	debug_assert((size_t)reliability >= 0 && (size_t)reliability < 2);
	auto &schedule = schedules[(size_t)reliability];
	
	auto sendQueueProcessingDelay =
		immediate ? 0 : options.delay_ms;
	auto now = connection->socket->service->clock.now();
	auto then = now + Duration(sendQueueProcessingDelay);
	
	
	auto lock = lock_of(schedule.mutex);
	
//	auto locked = connection->socket->service->clock.now();
//	auto timeItTookToLock = std::chrono::duration_cast<Duration>(locked - now).count();
//	sLogReleaseIf(timeItTookToLock > 1, "debug", logVar(timeItTookToLock));
	
	if (!schedule.when || then < *schedule.when)
	{
		schedule.when = then;

		if (!schedule.running)
		{
//			auto nextNow = std::chrono::duration_cast<Duration>(this->now() - now).count();
//			auto nexter = std::chrono::duration_cast<Duration>(*schedule.when - this->now()).count();
//			sLogRelease("debug", logOfThis(this) << logVar(reliability) << logVar(nexter) << logVar(nextNow) << logVar(sendQueueProcessingDelay));

			schedule.timeout.schedule(then);
		}
	}
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

void Sender::onReceive(Packet &packet)
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

void Sender::ack(PacketID packetID)
{
	{
		auto lock = lock_of(delayedAcksMutex);
		DelayedAck ack {
			packetID,
			connection->socket->service->clock.now()
		};
		delayedAcks[0].push_back(ack);
	}
	
	scheduleDataQueueProcessing(UNRELIABLE);
}

void Sender::queueDelayedAcks()
{
	auto now = connection->socket->service->clock.now();
	auto prev = lastDelayedAcksSend;
	
	while (true)
	{
		{
			auto lock = lock_of(delayedAcksMutex);
			if (delayedAcks[0].empty())
				break;

			lastDelayedAcksSend = now;
			
			debug_assert(delayedAcks[1].empty());
			std::swap(delayedAcks[0], delayedAcks[1]);
		}
		
		for (auto &delayedAck: delayedAcks[1])
		{
			auto delayedMS = std::chrono::duration_cast<Duration>(now - delayedAck.when).count();
			Ack ack {
				.packetID = delayedAck.packetID,
				.delayedMS = (u16)delayedMS
			} ;
			
			unreliableDataQueue.enqueue(
				ACK_FRAME,
				(u8 *)&ack, sizeof(ack),
				SendQueue::CoalesceMode::MRUDP_COALESCE_PACKET
			);
		}
		
		delayedAcks[1].clear();
	}
	
//	auto delayed = std::chrono::duration_cast<Duration>(now - prev).count();
//	if (delayed > 100 && delayed < 1000000)
//	{
//		int x = 0;
//		x++;
//	}
//
//	sLogRelease("mrudp::ack_failure",
//		logOfThis(this) << logLabelVar("delayed", std::chrono::duration_cast<Duration>(now - prev).count())
//	);
	
}

void Sender::onAck(const Packet &packet)
{
	onAck(Ack { packet.header.id, 0 });
}

void Sender::onAck(const Ack &ack)
{
	auto packetID = ack.packetID;
	
	auto now = connection->socket->service->clock.now();
	auto ackResult = retrier.ack(
		packetID,
		now,
		ack.delayedMS
	);

	if (ackResult.contained)
	{
		rtt.onSample(ackResult.rtt);
		
		sLogDebug("mrudp::rtt_computation", logOfThis(this) << logVar(rtt.duration) << logVar(ackResult.rtt) << logVar(windowSize.size));
		
		windowSize.onSample(rtt.duration);
		
		if (ackResult.needsRetryTimeoutRecalculation)
			retrier.recalculateRetryTimeout();
	}

	scheduleDataQueueProcessing(RELIABLE, true);
	connection->possiblyClose();
}

} // namespace
} // namespace
