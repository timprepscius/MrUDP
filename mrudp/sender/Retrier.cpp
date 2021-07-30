#include "Retrier.h"
#include "Sender.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Implementation.h"

namespace timprepscius {
namespace mrudp {

Retrier::Retrier (Sender *sender_) :
	sender(sender_)
{

}

Retrier::~Retrier ()
{
	close();
}

void Retrier::close ()
{
	auto lock = lock_of(mutex);
	
	if (status != CLOSED)
	{
		status = CLOSED;
		window.clear();
	}
}

StrongPtr<Retry> Retrier::getNextRetry()
{
	auto lock = lock_of(mutex);

	if (window.empty())
		return nullptr;
		
	return window.begin()->second;
}

Retrier::InsertResult Retrier::insert(const PacketPtr &packet, const Timepoint &now)
{
	bool wasEmpty = false;
	
	// lock
	{
		auto lock = lock_of(mutex);

		if (status == CLOSED)
			return { false };
			
		wasEmpty = window.empty();
		auto insertion = window.try_emplace(packet->header.id, strong<Retry>(Retry { packet, now }));
		debug_assert(insertion.second);
	}
	
	if (wasEmpty)
		recalculateRetryTimeout();
		
	return { wasEmpty };
}

Retrier::AckResult Retrier::ack(PacketID packetID, const Timepoint &now)
{
	auto lock = lock_of(mutex);

	bool wasFirst = false;
	bool contained = false;
	float rtt = 0;

	auto i = window.find(packetID);
	if (i != window.end())
	{
		contained = true;
		
		wasFirst = i == window.begin();
		auto &retry = i->second;

		auto duration = now - retry->sentAt;
		rtt = std::chrono::duration_cast<std::chrono::duration<float>>(duration).count();

		window.erase(i);
	}
	
	return {
		.contained = contained,
		.needsRetryTimeoutRecalculation = wasFirst,
		.rtt = rtt
	};
}

size_t Retrier::numUnacked()
{
	auto lock = lock_of(mutex);

	return window.size();
}

bool Retrier::empty ()
{
	auto lock = lock_of(mutex);
	return window.empty();
}

void Retrier::recalculateRetryTimeout()
{
	auto retry = getNextRetry();
	if (retry)
	{
		auto interval = toDuration(2 * sender->rtt.duration);

		retry->retryAt = retry->sentAt + interval;
		sender->connection->imp->setTimeout(
			"retry",
			retry->retryAt,
			[this]() {
				this->onRetryTimeout();
			}
		);
	}
}

void Retrier::onRetryTimeout()
{
	auto now = sender->connection->socket->service->clock.now();
	
	auto retry = getNextRetry();
	if (!retry)
		return;

	if (now > retry->retryAt)
	{
		xLogDebug(logOfThis(this) << logVar(retry));

		auto &header = retry->packet->header;
		(void)header;
		
		auto connection = sender->connection;
		
		if(retry->attempts >= maximumAttempts)
		{
			sLogDebug("mrudp::retry", logOfThis(this) << logLabelVar("local", toString(connection->socket->getLocalAddress())) << logLabelVar("remote", toString(connection->remoteAddress)) << logLabel("FAILING") << logVar(header.id) << logVar((char)header.type) << logVar(retry->attempts) << "rtt.duration " << sender->rtt.duration);

			xTraceChar(this, retry->packet->header.id, 'F', (char)retry->packet->header.type);
			connection->fail(MRUDP_EVENT_TIMEOUT);
		}
		else
		{
			retry->attempts++;
			retry->sentAt = connection->socket->service->clock.now();
			
			sLogDebug("mrudp::retry", logOfThis(this) << logLabelVar("local", toString(connection->socket->getLocalAddress())) << logLabelVar("remote", toString(connection->remoteAddress))<< logLabel("retrying") << logVar(header.id) << logVar((char)header.type) << logVar(retry->attempts) << "rtt.duration " << sender->rtt.duration);

			connection->resend(retry->packet);
			
			sender->rtt.onAckFailure();
			recalculateRetryTimeout();
		}
	}
	else
	{
		recalculateRetryTimeout();
	}
}

} // namespace
} // namespace
