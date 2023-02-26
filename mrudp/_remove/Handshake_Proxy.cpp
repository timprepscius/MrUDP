#include "Handshake_Proxy.h"
#include "connection/Probe.h"
#include "Connection.h"

namespace timprepscius {
namespace mrudp {

struct HandshakeOptionsData {
	uint32_t probe_delay_ms;
} ;

Handshake_Options::Handshake_Options(Connection *connection_) :
	connection(connection_)
{
}

PacketDiscard Handshake_Options::onSend (Packet &packet)
{
	if (packet.header.type == H0)
	{
		// this needs to be something different
		if (!pushData(packet, connection->proxy))
			return Discard;
			
		if (connection->proxy)
		{
			// TODO: check if this is correct
			if (!pushData(packet, connection->proxyAddress))
				return Discard;
		}
	}

	return Keep;
}

PacketDiscard Handshake_Options::onReceive (Packet &packet)
{
	if (packet.header.type == H0)
	{
		if (!popData(packet, connection->proxy))
			return Discard;
			
		if (connection->proxy)
		{
			// TODO: check if this is correct
			if (!popData(packet, connection->proxyAddress))
				return Discard;
		}
	}

	return Keep;
}


} // namespace
} // namespace
