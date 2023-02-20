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
struct Handshake_Options
{
	Handshake_Options(Connection *connection);
	Connection *connection;
	
	PacketDiscard onReceive (Packet &packet);
	PacketDiscard onSend (Packet &packet);
} ;

} // namespace
} // namespace
