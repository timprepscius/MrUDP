#pragma once

#include "Retrier.h"
#include "RTT.h"
#include "WindowSize.h"
#include "IDGenerator.h"
#include "SendQueue.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

// --------------------------------------------------------------------------------
// Sender
//
// Sender is the container for the sending mechanisms.
//
// When a packet is asked to be sent, it is first placed in the sendQueue and then
// the send queue is processed.
//
// If it is declared ok to send, a packet header is generated with a unique send id,
// it is inserted into the retrier for future use, and then sent on the connection.
//
// Notes:  The Sender::processSendQueue, is currently called directly after each
// packet is queued, however most likely I will put in a delay of X milliseconds
// and then call processSendQueue which will coalesce packets.
//
// When a packet is acked, Sender::onAck is called which in turn calls the
// Retrier::ack, and recalculates timeouts if necessary.  In addition, after an ack
// is received the sendQueue is processed (if more packets can be sent on a waiting
// connection), and the connection is possibly closed (if a fin ack was received).
//
// --------------------------------------------------------------------------------

struct Sender
{
	enum Status {
		UNINITIALIZED,
		OPEN,
		CLOSED
	} ;
	
	Status status = UNINITIALIZED;
	Connection *connection;
	
	Sender(Connection *connection_);

	IDGenerator<PacketID> packetIDGenerator;
	RTT rtt;
	WindowSize windowSize;
	Retrier retrier;
	SendQueue dataQueue;
	
	SendQueue unreliableDataQueue;
	
	void open ();
	bool isReadyToSend ();

	ErrorCode send(const u8 *data, size_t size, Reliability reliability);
	
	void sendReliably(const PacketPtr &packet);
	void onAck(Packet &packet);
	void onPacket (Packet &packet);
	void close ();
	void fail ();
	
	bool empty ();
	
	void enqueue(FrameTypeID type, const u8 *data, size_t size, Reliability reliability, SendQueue::CoalesceMode mode);
	
	void processDataQueue(Reliability reliability);
	void processReliableDataQueue ();
	void processUnreliableDataQueue ();
	
	struct Schedule
	{
		String name;
		Atomic<bool> waiting = false;
	} ;
	
	// scheduler for reliable and unreliable
	Schedule schedules[2];
	void scheduleDataQueueProcessing (Reliability reliability);
};

} // namespace
} // namespace
