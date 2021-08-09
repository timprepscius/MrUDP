#pragma once

#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"

#include "../Base.h"

#include "BoostAsio.h"

namespace timprepscius {
namespace mrudp {
namespace imp {

// --------------------------------------
// the possibilities
// #define MRUDP_IO_OVERLAPPED
// #define MRUDP_IO_DIRECT
// #define MRUDP_IO_SENDQUEUE
// --------------------------------------

#if defined(SYS_LINUX)
	// So far, I am not seeing the performance gain with
	// overlapped io. I must be doing something incorrectly.
	//
//	#define MRUDP_USE_OVERLAPPED_IO
	#define MRUDP_IO_SENDQUEUE
#else
//	#define MRUDP_THREAD_PER_CPU
	#define MRUDP_USE_OVERLAPPED_IO
//	#define MRUDP_IO_SENDQUEUE
	#define MRUDP_IO_DIRECT
#endif

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::system;

// --------------------------

mrudp_addr_t toAddr(const udp::endpoint &endpoint);
udp::endpoint toEndpoint(const mrudp_addr_t &addr);

struct Send
{
	Address address;
	PacketPtr packet;
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
		handle(service)
	{
	}
	
	udp::socket handle;
	SendQueue queue;
} ;

struct ServiceImp : StrongThis<ServiceImp>
{
	WeakPtr<Service> parent;
	List<Thread> runners;
	RecursiveMutex mutex;
	
	Mutex endpointsInUseMutex;
	Set<udp::endpoint> endpointsInUse;
	bool insertEndpoint(const udp::endpoint &endpoint);
	void eraseEndpoint(const udp::endpoint &endpoint);
	
	StrongPtr<io_service> service;
	StrongPtr<udp::resolver> resolver;
	io_service::work *working = nullptr;

	ServiceImp (Service *parent_);
	~ServiceImp ();
	
	void start ();
	void stop ();

	void resolve(const String &host, const String &port, Function<void(Vector<mrudp_addr_t> &&)> &&f);
} ;

struct ConnectionImp : StrongThis<ConnectionImp>
{
	WeakPtr<Connection> parent;
	OrderedMap<String, deadline_timer> timers;
	
	RecursiveMutex setTimeoutMutex;
	
#ifdef MRUDP_USE_OVERLAPPED_IO
	StrongPtr<SocketNative> connectedSocket;
#endif

	ConnectionImp(const StrongPtr<Connection> &parent_);
	~ConnectionImp();

	void open ();
	void stop ();

	void setTimeout (const String &name, const Timepoint &then, Function<void()> &&f);
} ;

struct SocketImp : StrongThis<SocketImp>
{
	WeakPtr<Socket> parent;

	StrongPtr<SocketNative> socket;
	
#ifdef MRUDP_USE_OVERLAPPED_IO
	Mutex connectedSocketsMutex;
	OrderedMap<Address, WeakPtr<SocketNative>> connectedSockets;
	StrongPtr<SocketNative> getConnectedSocket(const Address &);
	void releaseConnectedSocket(const Address &);
	
	void doReceive(const Address &remoteAddress, StrongPtr<SocketNative> &, const StrongPtr<Packet> &packet);
#endif

	Packet receivePacket;
	udp::endpoint localEndpoint, remoteEndpoint;
	void doReceiveFrom ();

	bool running = true;
	
	SocketImp(const StrongPtr<Socket> &parent, const Address &address);
	~SocketImp ();
	
	void open();
	void connect(const Address &address);
	void handleReceiveFrom(const Address &remoteAddress, Packet &receivePacket, size_t bytesTransferred);
	
	void send(const Address &addr, const PacketPtr &packet, Connection *connection);
	void sendDirect(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection);
	void sendViaQueue(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection);
	void doSend(const StrongPtr<SocketNative> &);
	void close ();
	
	mrudp_addr_t getLocalAddress ();
	
} ;

} // namespace
} // namespace
} // namespace
