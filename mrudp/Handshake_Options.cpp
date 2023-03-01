#include "Handshake_Options.h"
#include "connection/Probe.h"
#include "Connection.h"

namespace timprepscius {
namespace mrudp {

PACK (
	struct HandshakeOptionsData {
		uint32_t probe_delay_ms;
		int16_t maximum_retry_attempts;
	}
);

Handshake_Options::Handshake_Options(Connection *connection_) :
	connection(connection_)
{
}

PacketDiscard Handshake_Options::onSend (Packet &packet)
{
	if (packet.header.type == H3)
	{
		HandshakeOptionsData o {
			.probe_delay_ms = (uint32_t)std::max(connection->options.probe_delay_ms, 0),
			.maximum_retry_attempts =
				connection->options.maximum_retry_attempts
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
		connection->options.maximum_retry_attempts = o.maximum_retry_attempts;
	}

	return Keep;
}


} // namespace
} // namespace
