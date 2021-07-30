#pragma once

#include "Packet.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

struct Handshake
{
	Handshake(Connection *connection);

	Connection *connection;
	
	TypeID waitingFor = H0;
	PacketID firstNonHandshakePacketID = 0;
	void initiate();
	
	void handlePacket (Packet &packet);
	
	void onPacket(Packet &packet);
	void onHandshakeComplete ();
} ;

} // namespace
} // namespace
