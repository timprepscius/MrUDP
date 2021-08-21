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
	status(OPEN),
	probeInterval(5 * 1000)
{
}

void Probe::close ()
{
	status = CLOSED;
}

void Probe::onStart(const Timepoint &now)
{
	on(now);
	registerTimeout();
}

void Probe::on(const Timepoint &now)
{
	nextProbeTime = now + probeInterval;
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

void Probe::registerTimeout ()
{
	auto nextProbe = nextProbeTime;
	connection->imp->setTimeout(
		"probe",
		nextProbe,
		[this]() { this->onTimeout(); }
	);
}

void Probe::onTimeout()
{
	if (status == CLOSED)
		return;

	auto &sender = connection->sender;
	
	auto now = connection->socket->service->clock.now();
	auto nextProbe = nextProbeTime;

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
	
	registerTimeout();
}


} // namespace
} // namespace
