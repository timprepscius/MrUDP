#pragma once

#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"
#include "../Scheduler.h"

#include "../Base.h"

#include "BoostAsio.h"

namespace timprepscius {
namespace mrudp {
namespace imp {

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::system;

// --------------------------

mrudp_addr_t toAddr(const udp::endpoint &endpoint);
udp::endpoint toEndpoint(const mrudp_addr_t &addr);

typedef mrudp_options_asio_t OptionsImp;
void merge(OptionsImp &lhs, const OptionsImp &rhs);

OptionsImp getDefaultOptions();

struct Send
{
	Address address;
	PacketPtr packet;
} ;

struct Receive
{
	Packet packet;
	udp::endpoint endpoint;
} ;

struct SendQueue
{
	Mutex mutex;
	List<Send> queue;

	Send &front()
	{
		auto lk = lock_of(mutex);
		return queue.front();
	}
	
	size_t push_back(const Send &t)
	{
		auto lk = lock_of(mutex);
		queue.push_back(t);
		return queue.size();
	}

	size_t pop_front()
	{
		auto lk = lock_of(mutex);
		debug_assert(!queue.empty());
		queue.pop_front();
		return queue.size();
	}
} ;

struct SocketNative
{
	SocketNative(io_service &service) :
		handle(service),
		isOverlapped(false)
	{
	}

	SocketNative(
		io_service &service,
		const udp::endpoint &remoteEndpoint_,
		const WeakPtr<SocketImp> &socket_,
		const Address &overlapKey_) :
		
		handle(service),
		isOverlapped(true),
		remoteEndpoint(remoteEndpoint_),
		socket(socket_),
		overlapKey(overlapKey_)
	{
	}
	
	~SocketNative ();

	SharedMutex handleMutex;
	udp::socket handle;
	SendQueue queue;
	
	bool isOverlapped;
	udp::endpoint remoteEndpoint;

	WeakPtr<SocketImp> socket;
	Address overlapKey;
	
	void send(const Send &send, Function<void(const error_code &)> &&f);
	void receive(Receive &receive, Function<void(const error_code &)> &&f);
} ;

// ------------

struct SchedulerImp
{
	OptionsImp options;

	SchedulerImp(io_service &io, const OptionsImp *options);

	Scheduler *scheduler = nullptr;
	uint64_t nextExpiration = 0;
	steady_timer timer;

	void update(const Timepoint &when, bool isRequired);
	
	void process();
} ;

// ---------

struct ServiceImp : StrongThis<ServiceImp>
{
	OptionsImp options;

	WeakPtr<Service> parent;
	List<Thread> runners;
	RecursiveMutex mutex;

//	Mutex endpointsInUseMutex;
//	Set<udp::endpoint> endpointsInUse;
	bool insertEndpoint(const udp::endpoint &endpoint);
	void eraseEndpoint(const udp::endpoint &endpoint);
	
	StrongPtr<io_service> service;
	StrongPtr<udp::resolver> resolver;
	StrongPtr<SchedulerImp> scheduler;

	io_service::work *working = nullptr;

	ServiceImp (Service *parent_, const OptionsImp *options);
	~ServiceImp ();
	
	void start ();
	void stop ();

	void resolve(const String &host, const String &port, Function<void(Vector<mrudp_addr_t> &&)> &&f);
	
} ;

struct ConnectionImp : StrongThis<ConnectionImp>
{
	WeakPtr<Connection> parent;

//	RecursiveMutex setTimeoutMutex;
//	OrderedMap<String, deadline_timer> timers;
	
	StrongPtr<SocketNative> overlappedSocket;

	ConnectionImp(const StrongPtr<Connection> &parent_);
	~ConnectionImp();

	mrudp_error_code_t open ();
	void stop ();

	void reacquireOverlappedSocket ();
	void onRemoteAddressChanged();
	void relocate ();
} ;

struct SocketImp : StrongThis<SocketImp>
{
	OptionsImp options;

	WeakPtr<Socket> parent;

	StrongPtr<SocketNative> socket;
	udp::endpoint localEndpoint;
	
	Mutex overlappedSocketsMutex;
	OrderedMap<Address, WeakPtr<SocketNative>> overlappedSockets;
	StrongPtr<SocketNative> getOverlappedSocket(const Address &);
	void releaseOverlappedSocket(const Address &);

	bool running = false;
	void doReceive(const StrongPtr<SocketNative> &, const StrongPtr<Receive> &packet);
	
	SocketImp(const StrongPtr<Socket> &parent, const Address &address);
	~SocketImp ();
	
	void acquireAddress(const Address &address);
	
	void open();
	void connect(const Address &address);
	void handleReceiveFrom(const Address &remoteAddress, Packet &receivePacket);
	
	mrudp_error_code_t send(const PacketPtr &packet, Connection *connection, const Address *addr);
	void sendDirect(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection);
	void sendViaQueue(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection);
	void doSend(const StrongPtr<SocketNative> &);
	void close ();
	
	void relocate(const Address &address);
	
	mrudp_addr_t getLocalAddress ();
	
} ;

} // namespace
} // namespace
} // namespace
