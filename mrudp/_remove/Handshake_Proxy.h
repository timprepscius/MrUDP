#pragma once

#include "Packet.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

// --------------------------------------------------------
// Handshake_Proxy
//
// Sends proxy information along with the handshake
// --------------------------------------------------------
struct Handshake_Proxy
{
	Handshake_Proxy(Connection *connection);
	Connection *connection;
	
	PacketDiscard onReceive (Packet &packet);
	PacketDiscard onSend (Packet &packet);
} ;

} // namespace
} // namespace
