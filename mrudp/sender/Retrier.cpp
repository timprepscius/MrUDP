#include "Retrier.h"
#include "Sender.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Implementation.h"

namespace timprepscius {
namespace mrudp {

Retrier::Retrier (Sender *sender_) :
	sender(sender_),
	maximumAttempts(sender_->connection->options.maximum_retry_attempts)
{
	sender->connection->socket->service->scheduler->allocate(
		timeout,
		[this]() {
			this->onRetryTimeout();
		}
	);
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
		
	if (!priority.empty())
	{
		auto i = window.find(*priority.begin());
		debug_assert(i != window.end());
		return i->second;
	}
	
	return window.begin()->second;
}

Retrier::InsertResult Retrier::insert(const MultiPacketPath &packetPaths, const Timepoint &now, bool priority)
{
	bool wasEmpty = false;
	bool wasPriority = false;
	debug_assert(!packetPaths.empty());
	
	// scoped lock
	{
		auto lock = lock_of(mutex);

		if (status == CLOSED)
			return { false };
			
		wasEmpty = window.empty();
		
		auto id = packetPaths.front().packet->header.id;
		
		auto retry = strong<Retry>(Retry {
			.paths = packetPaths,
			.sentAt = now,
			.priority = priority
		});
		
		auto insertion = window.try_emplace(id, retry);
		debug_assert(insertion.second);
		
		if (priority)
		{
			this->priority.insert(id);
			wasPriority = true;
		}
	}
	
	if (wasEmpty || wasPriority)
		recalculateRetryTimeout();
		
	return { wasEmpty };
}

Retrier::AckResult Retrier::ack(PacketID packetID, const Timepoint &now, u16 delayedMS)
{
	auto lock = lock_of(mutex);

	bool wasFirst = false;
	bool wasPriority = false;
	bool contained = false;
	float rtt = 0;

	auto i = window.find(packetID);
	if (i != window.end())
	{
		contained = true;
		
		wasFirst = i == window.begin();
		auto &retry = i->second;

		auto duration = now - retry->sentAt - Duration(delayedMS);
		rtt = std::chrono::duration_cast<std::chrono::duration<float>>(duration).count();

		if (retry->priority)
		{
			wasPriority = true;

			auto j = priority.find(packetID);
			if (j != priority.end())
				priority.erase(j);
		}

		window.erase(i);
	}
	else
	{
		sLogRelease("debug", logOfThis(this) << "ack for nothing " << logVar(packetID));
	}
	
	return {
		.contained = contained,
		.needsRetryTimeoutRecalculation = wasFirst || wasPriority,
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

float Retrier::calculateRetryDuration(float rtt)
{
	auto delayedAckContribution = 30/1000.0f;
	return 2 * rtt + delayedAckContribution;
}

void Retrier::recalculateRetryTimeout()
{
	auto retry = getNextRetry();
	if (retry)
	{
		auto interval = toDuration(calculateRetryDuration(sender->rtt.duration));

		retry->retryAt = retry->sentAt + interval;
		timeout.schedule(retry->retryAt);
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
		
		auto connection = sender->connection;
		
		if(retry->attempts >= maximumAttempts)
		{
			auto &header = retry->paths.front().packet->header;
			(void)header;

			sLogDebug("mrudp::retry", logOfThis(this) << logLabelVar("local", toString(connection->socket->getLocalAddress())) << logLabelVar("remote", toString(connection->remoteAddress)) << logLabel("FAILING") << logVarV(header.id) << logVarV((char)header.type) << logVar(retry->attempts) << "rtt.duration " << sender->rtt.duration);

	
			xTraceChar(this, header.id, 'F', (char)header.type);
			connection->fail(MRUDP_EVENT_TIMEOUT);
		}
		else
		{
			sLogRelease("mrudp::ack_failure",
				logOfThis(this) << "ack failure " <<
				logLabelVarV("id", retry->paths.front().packet->header.id) <<
				logLabelVarV("duration", std::chrono::duration_cast<Duration>(now - retry->sentAt).count())
			);

			retry->attempts++;
			retry->sentAt = connection->socket->service->clock.now();

			sLogReleaseIf(retry->attempts > 8, "mrudp::retry::lots", logOfThis(this) << logVar(retry->attempts));

			for (auto &path: retry->paths)
			{
				auto &header = path.packet->header;
				(void)header;

				sLogDebug("mrudp::retry", logOfThis(this) << logLabelVar("local", toString(connection->socket->getLocalAddress())) << logLabelVar("remote", toString(connection->remoteAddress))<< logLabel("retrying") << logVarV(header.id) << logVarV((char)header.type) << logVar(retry->attempts) << "rtt.duration " << sender->rtt.duration);

				xTraceChar(this, path.packet->header.id, '0' + (char)retry->attempts, (char)path.packet->header.type);
				if (path.address)
				{
					connection->resend(path.packet, &*path.address);
				}
				else
				{
					connection->resend(path.packet);
				}
			}

			sender->rtt.onAckFailure();
			sender->windowSize.onSample(sender->rtt.duration);
			
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
