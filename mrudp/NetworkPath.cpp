#include "NetworkPath.h"
#include "Connection.h"
#include "Socket.h"
#include "Service.h"

#include "imp/Crypto.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

NetworkPath::NetworkPath(Connection *connection_) :
	connection(connection_)
{
#ifdef MRUDP_ENABLE_CRYPTO
	signer = generateSHAKey(*connection->socket->service->crypto->random);
#endif
}

bool NetworkPath::sendChallenge(const Address &address)
{
	// paths can only be two right now
	// if this changes, you need to modify MultiPacketPath
	MultiPacketPath packets {
		PacketPath { },
		PacketPath {
			.address = address
		}
	};
		
	for (auto &packetPath: packets)
	{
		packetPath.packet = strong<Packet>();
		packetPath.packet->header.type = AUTHENTICATE_CHALLENGE;
		
		Address address = {0};
		if (packetPath.address)
			address = *packetPath.address;

		auto now = connection->socket->service->clock.now();
		Time time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

		Vector<u8> basis;
		auto *timeBegin = (u8 *)&time;
		auto *timeEnd = timeBegin + sizeof(time);
		basis.insert(basis.end(), timeBegin, timeEnd);

		auto *addressBegin = (u8 *)&address;
		auto *addressEnd = addressBegin + sizeof(address);
		basis.insert(basis.end(), addressBegin, addressEnd);
		pushData(*packetPath.packet, basis);

		auto signature = Vector<u8>();

#ifdef MRUDP_ENABLE_CRYPTO
		signature.resize(signer->ByteSize);
		
		if (!signer->sign(basis.data(), basis.size(), signature.data(), signature.size()))
			return false;
#endif
			
		pushData(*packetPath.packet, signature);
	}
	
	connection->sender.sendReliablyMultipath(packets, true);
	return true;
}

bool NetworkPath::sendChallengeResponse(Packet &packet)
{
	auto ack = strong<Packet>();
	ack->header.type = AUTHENTICATE_RESPONSE;
	ack->header.id = packet.header.id;

	memcpy(ack->data, packet.data, packet.dataSize);
	ack->dataSize = packet.dataSize;

	connection->send(ack);
	return true;
}

bool NetworkPath::verifyChallengeResponse(Packet &packet, const Address &path)
{
	Vector<u8> signature;
	if (!popData(packet, signature))
		return false;
		
	Vector<u8> basis;
	if (!popData(packet, basis))
		return false;
	
	if (basis.size() != sizeof(Address) + sizeof(Time))
		return false;
		
	auto *sentTime = (Time *)basis.data();
	auto timeEnd = sentTime + 1;
	
	auto now = connection->socket->service->clock.now();
	Time receivedTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	
	// one minute
	Time maximumAllowedTime = 60;
	if (receivedTime - *sentTime > maximumAllowedTime)
		return false;
	
	auto *address = (Address *)timeEnd;
	Address empty = {0};
	
	// if the address is empty
	if (*address == empty)
	{
		// then the path it comes from must be the already established address
		if (path != connection->remoteAddress)
		{
			return false;
		}
	}
	// if the address is not empty
	else
	{
		// then it must match the path from which the packet came from
		if (*address != path)
		{
			return false;
		}
	}
	
#ifdef MRUDP_ENABLE_CRYPTO
	if (!signer->verify(basis.data(), basis.size(), signature.data(), signature.size()))
	{
		return false;
	}
#endif

	return true;
}

void NetworkPath::onReceive(Packet &packet, const Address &from)
{
	if (from != connection->remoteAddress)
	{
		if (packet.header.type == AUTHENTICATE_RESPONSE)
		{
			if (verifyChallengeResponse(packet, from))
			{
				connection->onRemoteAddressChanged(from);
			}
		}
		else
		{
			sendChallenge(from);
		}
	}
	
	if (packet.header.type == AUTHENTICATE_CHALLENGE)
	{
		sendChallengeResponse(packet);
	}
}

} // namespace
} // namespace
