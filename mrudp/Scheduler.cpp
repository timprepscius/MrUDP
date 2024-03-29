#include "Scheduler.h"

#include "Implementation.h"
#include <iostream>

#include "Base.h"

namespace timprepscius {
namespace mrudp {
namespace scheduler {

Timeout::~Timeout()
{
	if (scheduler)
		scheduler->free(*this);
}

void Timeout::schedule(const Timepoint &then)
{
	scheduler->schedule(*this, then);
}

Scheduler::Scheduler(Service *service_):
	service(service_)
{
}

void Scheduler::open ()
{
	imp = service->imp->scheduler;
	imp->scheduler = this;
}

void Scheduler::close ()
{
	if (imp)
		imp->scheduler = nullptr;
		
	imp = nullptr;
}

void Scheduler::allocate(Timeout &timeout, Callback &&callback)
{
	timeout.scheduler = this;
	
	auto lock = lock_of(mutex);
	
	timeout.handle =
		queue.extract(queue.emplace());
		
	auto &value = timeout.handle.value();
	value.parent = &timeout;
	value.f = std::move(callback);
		
	timeout.where = queue.end();
}

void Scheduler::free(Timeout &timeout)
{
	auto lock = lock_of(mutex);
	
	if (timeout.handle.empty())
		timeout.handle =
			queue.extract(timeout.where);
			
	timeout.scheduler = nullptr;
}

Tuple<bool, Timepoint> Scheduler::schedule_(Timeout &timeout, const Timepoint &then_)
{
	debug_assert(mutex.locked_by_caller());
	
	bool changed = false;

	if (timeout.handle.empty())
	{
		if (timeout.where == queue.begin())
			changed = true;

		timeout.handle = queue.extract(timeout.where);
	}
	
	auto ensureRuns = 0;
	auto then = then_;
	if (then <= last)
	{
		ensureRuns = 1;
		then = last;
	}
		
	auto duration = then.time_since_epoch();
	uint64_t milliseconds =  std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() + ensureRuns;
	
	auto roundAmount = 10 - (milliseconds % 10);
	then = then + std::chrono::milliseconds(roundAmount);
		
	timeout.handle.value().when = then;
	timeout.where = queue.insert(std::move(timeout.handle));
	
//	if (then == queue.begin()->when)
	if (timeout.where == queue.begin())
		changed = true;
		
	return { changed, queue.begin()->when };
}

void Scheduler::schedule(
	Timeout &timeout,
	const Timepoint &then
)
{
	PROFILE_FUNCTION(this);

	auto lock = lock_of(mutex);
	
	auto [changed, next] = schedule_(timeout, then);
	
	if (changed)
		imp->update(next, false);
}

Timepoint Scheduler::process_(const Timepoint &now)
{
	PROFILE_FUNCTION(this);

	mutex.lock();
	last = now;
	
	while (true)
	{
		if (queue.empty())
			break;
			
		auto &front = *queue.begin();
		if (front.when >= now)
			break;

		auto *timeout = front.parent;
		timeout->handle = queue.extract(timeout->where);
		mutex.unlock();
		
		front.f();

		mutex.lock();
	}
	
	Timepoint next = Timepoint::min();
	
	if (!queue.empty())
		next = queue.begin()->when;
	
	mutex.unlock();
	return next;
}

void Scheduler::process ()
{
	auto self = strong_this(this);
	
	auto time = service->clock.now();
	auto next = process_(time);
	
	if (next != Timepoint::min())
	if (imp)
		imp->update(next, true);
}


} // namespace
} // namespace
} // namespace
