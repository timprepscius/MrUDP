#pragma once

#include "Retrier.h"
#include "RTT.h"
#include "WindowSize.h"
#include "SendIDGenerator.h"
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

	SendIDGenerator sendIDGenerator;
	RTT rtt;
	WindowSize windowSize;
	Retrier retrier;
	SendQueue dataQueue;
	
	SendQueue unreliableDataQueue;
	
	bool isUninitialized ();
	
	void open ();

	void send(const u8 *data, size_t size, Reliability reliability);
	
	void sendImmediately(const PacketPtr &packet);
	void onAck(Packet &packet);
	void onPacket (Packet &packet);
	void close ();
	void fail ();
	
	bool empty ();
	
	void processSendQueue ();
	void scheduleSendQueueProcessing ();
};

} // namespace
} // namespace
