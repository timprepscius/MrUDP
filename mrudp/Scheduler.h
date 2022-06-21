#pragma once

#include "Base.h"
#include <functional>

namespace timprepscius {
namespace mrudp {

struct Service;

namespace imp {
	struct SchedulerImp;
}

namespace scheduler {

using Callback = std::function<void()>;

struct Timeout;

struct Node {
	Timepoint when;
	Callback f;
	
	Timeout *parent;
} ;

using Queue = std::multiset<Node>;


inline
bool operator<(const Node &lhs, const Node &rhs)
{
	return lhs.when < rhs.when;
}

struct Scheduler;

struct Timeout
{
	~Timeout();

	Scheduler *scheduler = nullptr;

	Queue::iterator where;
	Queue::node_type handle;
	
	void schedule(const Timepoint &then);
} ;

struct Scheduler : StrongThis<Scheduler>
{
	Scheduler(Service *service_);
	
	Service *service;

	void open();
	void close();

	StrongPtr<imp::SchedulerImp> imp;
	Mutex mutex;
	Queue queue;
	
	Timepoint last = Timepoint::min();

	void allocate(Timeout &timeout, Callback &&callback);
	void free (Timeout &timeout);
	
	// returns the first timeout time, and whether it changed
	Tuple<bool, Timepoint> schedule_(
		Timeout &timeout,
		const Timepoint &then
	);
	
	void schedule(
		Timeout &timeout,
		const Timepoint &then
	);
	
	Timepoint process_(const Timepoint &now);
	
	void process();
} ;

} // namespace

using Scheduler = scheduler::Scheduler;
using Timeout = scheduler::Timeout;

} // namespace
} // namespace
