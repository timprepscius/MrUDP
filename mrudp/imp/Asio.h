#pragma once

#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"

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
		isConnected(false)
	{
	}

	SocketNative(io_service &service, const udp::endpoint &remoteEndpoint_) :
		handle(service),
		isConnected(true),
		remoteEndpoint(remoteEndpoint_)
	{
	}

	udp::socket handle;
	SendQueue queue;

	bool isConnected;
	udp::endpoint remoteEndpoint;
	
	void send(const Send &send, Function<void(const error_code &)> &&f);
	void receive(Receive &receive, Function<void(const error_code &)> &&f);
} ;

struct ServiceImp : StrongThis<ServiceImp>
{
	OptionsImp options;

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

	ServiceImp (Service *parent_, const OptionsImp *options);
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
	
	StrongPtr<SocketNative> connectedSocket;

	ConnectionImp(const StrongPtr<Connection> &parent_);
	~ConnectionImp();

	void open ();
	void stop ();

	void setTimeout (const String &name, const Timepoint &then, Function<void()> &&f);
} ;

struct SocketImp : StrongThis<SocketImp>
{
	OptionsImp options;

	WeakPtr<Socket> parent;

	StrongPtr<SocketNative> socket;
	udp::endpoint localEndpoint;
	
	Mutex connectedSocketsMutex;
	OrderedMap<Address, WeakPtr<SocketNative>> connectedSockets;
	StrongPtr<SocketNative> getConnectedSocket(const Address &);
	void releaseConnectedSocket(const Address &);

	bool running = true;
	void doReceive(const StrongPtr<SocketNative> &, const StrongPtr<Receive> &packet);
	
	SocketImp(const StrongPtr<Socket> &parent, const Address &address);
	~SocketImp ();
	
	void open();
	void connect(const Address &address);
	void handleReceiveFrom(const Address &remoteAddress, Packet &receivePacket);
	
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
