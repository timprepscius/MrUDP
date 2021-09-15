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

Retrier::AckResult Retrier::ack(PacketID packetID, const Timepoint &now)
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

		auto duration = now - retry->sentAt;
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
	return 2 * rtt;
}

void Retrier::recalculateRetryTimeout()
{
	auto retry = getNextRetry();
	if (retry)
	{
		auto interval = toDuration(calculateRetryDuration(sender->rtt.duration));

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
		
		auto connection = sender->connection;
		
		if(retry->attempts >= maximumAttempts)
		{
			auto &header = retry->paths.front().packet->header;
			(void)header;

			sLogDebug("mrudp::retry", logOfThis(this) << logLabelVar("local", toString(connection->socket->getLocalAddress())) << logLabelVar("remote", toString(connection->remoteAddress)) << logLabel("FAILING") << logVar(header.id) << logVar((char)header.type) << logVar(retry->attempts) << "rtt.duration " << sender->rtt.duration);

	
			xTraceChar(this, header.id, 'F', (char)header.type);
			connection->fail(MRUDP_EVENT_TIMEOUT);
		}
		else
		{
			retry->attempts++;
			retry->sentAt = connection->socket->service->clock.now();
			
			for (auto &path: retry->paths)
			{
				auto &header = path.packet->header;
				(void)header;

				sLogDebug("mrudp::retry", logOfThis(this) << logLabelVar("local", toString(connection->socket->getLocalAddress())) << logLabelVar("remote", toString(connection->remoteAddress))<< logLabel("retrying") << logVar(header.id) << logVar((char)header.type) << logVar(retry->attempts) << "rtt.duration " << sender->rtt.duration);

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
