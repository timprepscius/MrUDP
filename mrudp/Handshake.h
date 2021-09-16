#pragma once

#include "Packet.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

// --------------------------------------------------------
// Handshake
//
// Handshake reacts to handshake packets, providing appropriate
// acks.
//
// Random notes:
// I experimented with waitingFor as an atomic, and not using a mutex;
// however I made a minor mistake within initiate, where I did
// not set the waitingFor before sending the packet.  This would cause
// a race condition..  At this point I figure a mutex
// is simpler to use and less error prone.
// --------------------------------------------------------
struct Handshake
{
	Handshake(Connection *connection);

	Connection *connection;
	
	Mutex mutex;
	TypeID waitingFor = H0;
	PacketID firstNonHandshakePacketID = 0;
	void initiate();
	
	void handlePacket (Packet &packet);
	void onHandshakeComplete ();

	PacketDiscard onReceive(Packet &packet);
} ;

} // namespace
} // namespace
