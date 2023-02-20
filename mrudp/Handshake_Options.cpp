#include "Handshake_Options.h"
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
	if (packet.header.type == H3)
	{
		HandshakeOptionsData o {
			.probe_delay_ms = (uint32_t)std::max(connection->options.probe_delay_ms, 0)
		};
		
		if (!pushData(packet, o))
			return Discard;
	}

	return Keep;
}

PacketDiscard Handshake_Options::onReceive (Packet &packet)
{
	if (packet.header.type == H3)
	{
		HandshakeOptionsData o;
		if (!popData(packet, o))
			return Discard;
			
		connection->options.probe_delay_ms = o.probe_delay_ms;
	}

	return Keep;
}


} // namespace
} // namespace
