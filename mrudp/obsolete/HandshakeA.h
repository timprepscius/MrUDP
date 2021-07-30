#pragma once

#include "Packet.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

struct HandshakeA
{
	HandshakeA(Connection *connection);

	Connection *connection;
	
	TypeID step = H0;
	PacketID firstNonHandshakePacketID = 0;
	void initiate();
	
	void sendPacket (TypeID type, PacketID packetID);
	void receivePacket (Packet &packet);
	
	void onPacket(Packet &packet);
	void onHandshakeComplete ();
} ;

} // namespace
} // namespace
