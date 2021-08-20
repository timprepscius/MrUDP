#include "Statistics.h"

namespace timprepscius {
namespace mrudp {

ConnectionStatistics::ConnectionStatistics () :
	statistics({0})
{
}

void ConnectionStatistics::onReceive (Packet &packet)
{
	if (packet.header.type == DATA_RELIABLE)
		statistics.reliable.packets.received++;

	if (packet.header.type == DATA_UNRELIABLE)
		statistics.unreliable.packets.received++;

	statistics.packets_received++;
}

void ConnectionStatistics::onSend (Packet &packet)
{
	if (packet.header.type == DATA_RELIABLE)
		statistics.reliable.packets.sent++;

	if (packet.header.type == DATA_UNRELIABLE)
		statistics.unreliable.packets.sent++;
		
	statistics.packets_sent++;
}

void ConnectionStatistics::onReceiveDataFrame (int size, Reliability reliability)
{
	if (reliability == RELIABLE)
	{
		statistics.reliable.frames.received++;
		statistics.reliable.bytes.received += size;
	}
	else
	{
		statistics.unreliable.frames.received++;
		statistics.unreliable.bytes.received += size;
	}
}

void ConnectionStatistics::onSendDataFrame (int size, Reliability reliability)
{
	if (reliability == RELIABLE)
	{
		statistics.reliable.frames.sent++;
		statistics.reliable.bytes.sent += size;
	}
	else
	{
		statistics.unreliable.frames.sent++;
		statistics.unreliable.bytes.sent += size;
	}
}

void ConnectionStatistics::onResend(Packet &packet)
{
	statistics.packets_resent++;
}

const mrudp_connection_statistics_t &ConnectionStatistics::query()
{
	return statistics;
}

} // namespace
} // namespace
