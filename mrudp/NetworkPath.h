#pragma once

#include "Packet.h"
#include "base/Core.h"

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
struct NetworkPath
{
	NetworkPath(Connection *connection);
	
	Connection *connection;
	
	void sendChallenge(const Vector<Address> &paths);
	void sendChallengeResponse(Packet &challenge);
	
	bool verifyChallengeResponse(Packet &packet, const Address &path);
	
	void onReceive(Packet &packet, const Address &from);
} ;

} // namespace
} // namespace
