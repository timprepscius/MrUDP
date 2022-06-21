#pragma once

#include "../Base.h"
#include "../Scheduler.h"

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
	
	// Probe interval is dependent on rtt such that
	// a probe must be sent before the final retry is determined to have failed
	// or rather, it must be sent out at the time a packet would be on the last
	// retry.
	//
	// Hmmm, this is not a very clear explanation.
	//
	// Basically it goes like this:
	// Client A connects to server B
	// Server B sends an update packet X to client A
	// however client A has changed IP, because of some network path change
	//
	// Packet X is going to be retried N times
	// and after the Nth time expires the connection between the server
	// and the client will timeout
	//
	// To prevent this, the client must send a probe packet at the time
	// when the packet X's (N-1)th retry.
	//
	// Since the retry is based on rtt, and the probe is based on the retry
	// then the probe is based on rtt.	
	Duration probeInterval;
	
	Timeout timeout;
	
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
	bool isRegistered = false;
	void registerTimeout (const Timepoint &at);
	
	// decides whether or not to send a probe at a given time
	void onTimeout();
	
	// calculates how much time will elapse until the (last-1) retry of a packet sent exactly when the
	// last event was was
	void recalculateProbeTimeout();
} ;


} // namespace
} // namespace
