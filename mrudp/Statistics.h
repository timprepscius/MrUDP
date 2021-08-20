#pragma once

#include "Packet.h"

namespace timprepscius {
namespace mrudp {

struct ConnectionStatistics
{
	mrudp_connection_statistics_t statistics;
	
	// need to use a better name for this method
	const mrudp_connection_statistics_t &query();
	
	ConnectionStatistics ();
	
	void onReceive (Packet &packet);
	void onSend (Packet &packet);
	
	void onReceiveDataFrame (int size, Reliability reliability);
	void onSendDataFrame (int size, Reliability reliability);
	
	void onResend (Packet &packet);
} ;

} // namespace
} // namespace
