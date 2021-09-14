#include "NetworkPath.h"
#include "Connection.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

NetworkPath::NetworkPath(Connection *connection_) :
	connection(connection_)
{
	
}

void NetworkPath::challengePaths(const Vector<Address> &addresses)
{
	// paths can only be two right now
	// if this changes, you need to modify MultiPacketPath
	debug_assert(addresses.size() == 2);
	
	MultiPacketPath packets;
	for (auto &address: addresses)
	{
		PacketPath packetPath;
		packetPath.address = address;
		packetPath.packet = strong<Packet>();
		packetPath.packet->header.type = AUTHENTICATE_CHALLENGE;
		
		// TODO:
		// this should
		// 1. generate a random number
		// 2. encrypt the random number with a secret key
		// 3. push the random number and the encrypted number on the packet
		
		packets.push_back(std::move(packetPath));
	}
	
	connection->sender.sendReliablyMultipath(packets, true);
}

bool NetworkPath::verifyChallengeResponse(Packet &packet, const Address &path)
{
	// TODO:
	// this should
	// 1. pop the encrypted number and random number
	// 2. re-encrypt the random number with the secret key
	// 3. verify that the encrypted number matches

	return true;
}

void NetworkPath::onReceive(Packet &packet, const Address &from)
{
	if (from != connection->remoteAddress)
	{
		if (packet.header.type != AUTHENTICATE_RESPONSE)
		{
			challengePaths(Vector<Address> { connection->remoteAddress, from });
		}
		else
		{
			if (verifyChallengeResponse(packet, from))
			{
				// lock some mutex
				connection->remoteAddress = from;
			}
		}
	}
}

} // namespace
} // namespace
