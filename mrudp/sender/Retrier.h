#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// Retrier
//
// For reliability, the socket must retry packets that do not ack in an allowed
// period of time. This is done by maintaining a window of all unacked packets, as
// a map from their PacketID to the Retry structure.
//
// When a reliable packet is sent, it is first entered into the Retrier via
// Retrier::insert.
//
// When an ack is received for a reliable packet, the Retry is removed using
// Retrier::ack.
//
// When a Retry is either inserted or acked for the Retrier, a calculation may be
// required of when a check should be made to retry the first waiting packet.
//
// The calculation is made within Retrier::recalculateRetryTimeout
// The decision of retrying a packet, or timing a connection out is made in Retrier::onRetryTimeout
// --------------------------------------------------------------------------------

struct Sender;

struct Retry
{
	MultiPacketPath paths;
	Timepoint sentAt, retryAt;
	
	size_t attempts = 0;
	bool priority = false;
} ;

//inline
//bool operator<(const RetryPaths &lhs, const RetryPaths &rhs)
//{
//	auto &l = std::get<0>(lhs);
//	auto &r = std::get<0>(rhs);
//
//	if (l.priority != r.priority)
//		return l.priority < r.priority;
//
//	return l.sentAt > r.sentAt;
//}

struct Retrier
{
	enum Status {
		OPEN,
		CLOSED
	} ;
	
	Status status = OPEN;

	Retrier (Sender *sender);
	~Retrier ();
	
	Sender *sender;
	size_t maximumAttempts = 5;

	Mutex mutex;

	// The window of reliable packets that have been sent, but not acked.
	OrderedMap<PacketID, StrongPtr<Retry>> window;
	Set<PacketID> priority;
	
	StrongPtr<Retry> getNextRetry();

	struct InsertResult {
		bool wasFirst;
	} ;
	
	// Inserts a packet into the Retrier.
	// Returns the InsertResult, where .wasFirst signifies that
	// the recalculateRetryTimeout should be called
	InsertResult insert(const MultiPacketPath &packetPaths, const Timepoint &now, bool priority);
	
	struct AckResult
	{
		bool contained;
		bool needsRetryTimeoutRecalculation;
		float rtt;
	} ;
	
	// Signals to the Retrier that a packet was acked,
	// which causes the Retry to removed
	// Returns AckResult, where .needsRetryTimeoutRecalculation signifies
	// that recalculateRetryTimeout should be called
	AckResult ack(PacketID, const Timepoint &now);
	
	// returns the number of outstanding unacked packets
	size_t numUnacked();
	
	// clears the any waiting retries
	void close ();
	
	// returns if there are any outstanding unacked packets
	bool empty();
	
	// recalculates the retry timeout for the getNextRetry packet
	void recalculateRetryTimeout ();
	
	// calculates the retry duration given an rtt
	float calculateRetryDuration(float rtt);
	
	// does the retry mechanism, after checking that it should be done
	void onRetryTimeout ();
};

} // namespace
} // namespace
