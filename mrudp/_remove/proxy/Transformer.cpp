#include "Transformer.h"

namespace timprepscius {
namespace mrudp {
namespace proxy {

bool Transformer::receive_(Packet &packet, ProxyID &proxyID)
{
	proxyID = packet.header.connection;
	
	if (!popData(packet, (u8 *)&packet.header.connection, sizeof(packet.header.connection)))
		return false;
		
	if (!popData(packet, packet.header.type))
		return false;

	return true;
}

bool Transformer::receiveForward(Packet &packet, ProxyID &proxyID)
{
	if (packet.header.type == PROXY_FORWARD)
		return receive_(packet, proxyID);

	return false;
}

bool Transformer::receiveBackward(Packet &packet, ProxyID &proxyID)
{
	if (packet.header.type == PROXY_BACKWARD)
		return receive_(packet, proxyID);

	return false;
}

bool Transformer::send_(Packet &packet)
{
	if (!pushData(packet, packet.header.type))
		return false;
		
	if (!pushData(packet, packet.header.connection))
		return false;

	return true;
}

bool Transformer::sendForward(Packet &packet, const ProxyID &proxyID)
{
	send_(packet);
	packet.header.type = PROXY_FORWARD;
	packet.header.connection = proxyID;
	
	return true;
}

bool Transformer::sendBackward(Packet &packet, const ProxyID &proxyID)
{
	send_(packet);
	packet.header.type = PROXY_BACKWARD;
	packet.header.connection = proxyID;
	
	return true;
}

bool Transformer::sendMagic (Packet &packet, ProxyMagic magic)
{
	packet.header.type = PROXY_MAGIC;
	packet.dataSize = 0;
	if (!pushData(packet, magic))
		return false;
	
	return true;
}

bool Transformer::receiveMagic (Packet &packet, ProxyMagic expected)
{
	if (packet.header.type != PROXY_MAGIC)
		return false;
		
	ProxyMagic magic;
	if (!popData(packet, magic))
		return false;
		
	if (magic != expected)
		return false;
		
	return true;
}

} // namespace
} // namespace
} // namespace
