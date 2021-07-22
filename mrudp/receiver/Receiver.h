#pragma once

#include "ReceiveQueue.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

// --------------------------------------------------------------------------------
// Receiver
//
// Receiver is the container for the receiving mechanisms.
//
// When a packet is received, Receiver::onPacket inspects the packet and decides
// whether to open (SYN), close(FIN), process (DATA, PROBE), or process
// individually (DATA_UNRELIABLE)
//
// Processing is done as well within onPacket, if the header id is the next header
// id we are expecting, it processes immediately, if it is not, it queues it in
// an ordered processing queue.
// --------------------------------------------------------------------------------

struct Receiver
{
	enum Status {
		UNINITIALIZED,
		OPEN,
		CLOSED
	} ;
	
	Status status = UNINITIALIZED;
	Connection *connection;
	
	Receiver(Connection *connection);
	
	ReceiveQueue receiveQueue;
	
	// Called on a SYN packet, sets the status to Open, and
	// sets the expected packet id.
	void open (PacketID);
	
	// Called when a the user will no longer read from the receiver
	void close ();

	// Called when the connection is already failed
	void fail();
	
	// Dispatches to either reliable, unreliable, or probe paths
	void processReceived(Packet &packet);
	
	// Dispatches waiting packets if any are wiating
	void processReceiveQueue ();
	
	// Processes incoming packets
	void onPacket (Packet &packet);
};

} // namespace
} // namespace
