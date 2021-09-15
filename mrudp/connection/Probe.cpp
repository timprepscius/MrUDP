#include "Probe.h"

#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"

#include "../Implementation.h"

#include <iostream>

namespace timprepscius {
namespace mrudp {

Probe::Probe(Connection *connection_) :
	connection(connection_),
	status(OPEN)
{
}

void Probe::close ()
{
	status = CLOSED;
}

void Probe::onStart(const Timepoint &now)
{
	on(now);
}

void Probe::on(const Timepoint &now)
{
	recalculateProbeTimeout();
	registerTimeout(now + probeInterval);
}

void Probe::onReceive(const Timepoint &now)
{
	on(now);
}

void Probe::onSend(const Timepoint &now)
{
	on(now);
}

void Probe::onProbe(const Timepoint &now)
{
	on(now);
}

void Probe::registerTimeout (const Timepoint &at)
{
	auto set = at < nextProbeTime || !isRegistered;
	nextProbeTime = at;
	
	if (set)
	{
		isRegistered = true;
		
		connection->imp->setTimeout(
			"probe",
			nextProbeTime,
			[this]() { this->onTimeout(); }
		);
	}
}

void Probe::onTimeout()
{
	if (status == CLOSED)
		return;

	auto &sender = connection->sender;
	
	auto now = connection->socket->service->clock.now();
	auto nextProbe = nextProbeTime;
	isRegistered = false;

	if (now > nextProbe)
	{
		onProbe(now);
		
		if (sender.status == Sender::OPEN)
		{
			auto packet = strong<Packet>();
			packet->header.type = PROBE;
			
			sender.sendReliably(packet);
		}
		else
		{
			// when there is no probe sent
			// nothing can time out, but if the probe can't
			// even time out, it means the connection should fail
			// immediately
			connection->fail(MRUDP_EVENT_TIMEOUT);
		}
	}
	else
	{
		registerTimeout(nextProbe);
	}
	
}

void Probe::recalculateProbeTimeout ()
{
	auto &rtt_ = connection->sender.rtt;
	auto &retrier = connection->sender.retrier;
	
	// we don't know what they other side thinks rtt is, so we have to assume the worst
	// maybe rtt could be included in acks
	auto rtt = rtt_.minimum;
	
	auto timeout = retrier.calculateRetryDuration(rtt);
	auto numAttemptsBeforeProbe = (connection->sender.retrier.maximumAttempts + 1) / 2;
	for (auto i=0; i<numAttemptsBeforeProbe; ++i)
	{
		rtt = rtt_.calculate(rtt, rtt_.maximum);
		timeout += retrier.calculateRetryDuration(rtt);
	}

	probeInterval = toDuration(timeout);
	sLogDebug("mrudp::probe", logOfThis(this) << " " << logVar(timeout));
}


} // namespace
} // namespace
