#pragma once

#include "../Base.h"

namespace timprepscius {
namespace mrudp {

struct Connection;

// --------------------------------------------------------------------------------
// Probe
//
// Probe is responsible for, when there has not been packet transfer for a given
// period of time, sending out a probe packet that will establish whether the
// connection is still open, and keep current the rtt.
// --------------------------------------------------------------------------------

struct Probe
{
	enum Status {
		OPEN,
		CLOSED
	} ;
	Status status;

	Connection *connection;
	Timepoint nextProbeTime;
	Duration probeInterval;
	
	Probe (Connection *connection_);
	
	void close ();
	
	// updates the next probe time based on a packet
	// event occurring at now
	void on(const Timepoint &now);
	
	// All of these call ::on
	void onReceive(const Timepoint &now);
	void onStart(const Timepoint &now);
	void onSend(const Timepoint &now);
	void onProbe(const Timepoint &now);
	
	// registers when the probe timeout with the imp
	void registerTimeout ();
	
	// decides whether or not to send a probe at a given time
	void onTimeout();
} ;


} // namespace
} // namespace
