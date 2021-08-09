#include "Asio.h"

#include "../Base.h"
#include <boost/asio.hpp>
#include <iostream>

namespace timprepscius {
namespace mrudp {
namespace imp {

// --------------------------

typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> overlap_socket;
//typedef boost::asio::socket_base::reuse_address overlap_socket;

mrudp_addr_t toAddr(const udp::endpoint &endpoint)
{
	mrudp_addr_t address_;
	memset(&address_, 0, sizeof(address_));

	if (endpoint.protocol() == udp::v4())
	{
		auto address = endpoint.address();

		auto bytes = address.to_v4().to_bytes();
		#ifndef SYS_LINUX
			address_.v4.sin_len = sizeof(address_.v4);
		#endif
		address_.v4.sin_family = AF_INET;
		memcpy(&address_.v4.sin_addr, bytes.data(), bytes.size());
		address_.v4.sin_port = htons(endpoint.port());
	}
	else
	{
		auto address = endpoint.address();
		auto bytes = address.to_v6().to_bytes();
		#ifndef SYS_LINUX
			address_.v6.sin6_len = sizeof(address_.v6);
		#endif
		address_.v6.sin6_family = AF_INET6;
		memcpy(&address_.v6.sin6_addr, bytes.data(), bytes.size());
		address_.v6.sin6_port = htons(endpoint.port());
	}
	
	return address_;
}

udp::endpoint toEndpoint(const mrudp_addr_t &addr)
{
	if (addr.v4.sin_family == AF_INET)
	{
		address_v4::bytes_type bytes;
		memcpy(bytes.data(), &addr.v4.sin_addr, bytes.size());
		address_v4 a(bytes);
		
		return udp::endpoint(a, ntohs(addr.v4.sin_port));
	}
	else
	{
		address_v6::bytes_type bytes;
		memcpy(bytes.data(), &addr.v6.sin6_addr, bytes.size());
		address_v6 a(bytes);
		
		return udp::endpoint(a, ntohs(addr.v6.sin6_port));
	}
}

// --------------------------

ServiceImp::ServiceImp (Service *parent_) :
	parent(weak_this(parent_))
{
	service = strong<io_service>();
	resolver = strong<udp::resolver>(*service);
}

ServiceImp::~ServiceImp ()
{
	stop();
}

bool ServiceImp::insertEndpoint(const udp::endpoint &endpoint)
{
	auto lock = lock_of(endpointsInUseMutex);
	auto i = endpointsInUse.find(endpoint);
	
	if (i != endpointsInUse.end())
	{
		sLogDebug("mrudp::overlap_io", "endpoint already taken! " << endpoint);

		return false;
	}
		
	endpointsInUse.insert(endpoint);
	return true;
}

void ServiceImp::eraseEndpoint(const udp::endpoint &endpoint)
{
	auto lock = lock_of(endpointsInUseMutex);
	auto i = endpointsInUse.find(endpoint);

	if (i != endpointsInUse.end())
		endpointsInUse.erase(i);
}


void ServiceImp::start ()
{
	if (!working)
	{
		working = new io_service::work(*service);
		
		size_t processor_count = 1;

		#ifdef MRUDP_THREAD_PER_CPU
		processor_count = std::thread::hardware_concurrency();
		#endif
		
		std::cout << "starting " << processor_count << " io threads" << std::endl;
		for (auto i=0; i<processor_count; ++i)
		{
			runners.emplace_back([this, service=this->service]() {
				service->run();
			});
		}
	}
}

void ServiceImp::stop ()
{
	if (working)
	{
		delete working;
		working = nullptr;
		
		service->stop();
		
		for (auto &runner : runners)
		{
			if (runner.joinable())
			{
				auto runnerThread = runner.get_id();
				auto thisThread = std::this_thread::get_id();
				
				if (runnerThread != thisThread)
				{
					runner.join();
				}
				else
				{
					runner.detach();
				}
			}
		}
	}
}

void ServiceImp::resolve(const String &host, const String &port, Function<void(Vector<mrudp_addr_t> &&)> &&f)
{
	resolver->async_resolve(
		host, port,
		[f=std::move(f)](
			const error_code& ec,
			udp::resolver::results_type results
		)
		{
			std::vector<mrudp_addr_t> addresses;
			for (auto &result : results)
			{
				auto endpoint = result.endpoint();
				addresses.push_back(toAddr(endpoint));
			}
			
			f(std::move(addresses));
		}
	);
}

// -------------------------------------


ConnectionImp::ConnectionImp(const StrongPtr<Connection> &parent_) :
	parent(weak(parent_))
{
	xLogDebug(logOfThis(this));
	
#ifdef MRUDP_USE_OVERLAPPED_IO
	connectedSocket = parent_->socket->imp->getConnectedSocket(parent_->remoteAddress);
#endif
}

ConnectionImp::~ConnectionImp()
{
	xLogDebug(logOfThis(this));
	stop();
}

void ConnectionImp::open ()
{
}

void ConnectionImp::stop ()
{
	xLogDebug(logOfThis(this));

	auto lock = lock_of(setTimeoutMutex);
	for (auto &[key, timer] : timers)
		timer.cancel();

	timers.clear();
}

void ConnectionImp::setTimeout (const String &name, const Timepoint &then, Function<void()> &&f)
{
	xLogDebug(logOfThis(this));

	auto parent = strong(this->parent);
	if (!parent)
		return;

	auto lock = lock_of(setTimeoutMutex);
	
	auto [i, inserted] = timers.emplace(name, *parent->socket->service->imp->service);
	auto &timer = i->second;

	auto durationFromNow = then - parent->socket->service->clock.now();
	auto durationInMS = std::chrono::duration_cast<std::chrono::milliseconds>(durationFromNow).count();

	xLogDebug(logOfThis(this) << logLabel("setting timeout") << logVar(durationInMS));

	timer.expires_from_now(boost::posix_time::milliseconds(durationInMS));
	timer.async_wait([weak_self = weak_this(this), this, f=std::move(f)](auto ec) {
		if (!ec)
		if (auto self = strong(weak_self))
		if (auto parent = strong(self->parent))
		{
			f();
		}
	});
}

// -------------------------------------

SocketImp::SocketImp(const StrongPtr<Socket> &parent_, const Address &address) :
	parent(weak(parent_)),
	socket(strong<SocketNative>(*parent_->service->imp->service))
{
	bool acquiredAddress = false;
	while (!acquiredAddress)
	{
		socket->handle.close();
		
		auto endpoint = toEndpoint(address);
		socket->handle.open(endpoint.protocol());

		socket->handle.bind(endpoint);
		localEndpoint = socket->handle.local_endpoint();
		
		acquiredAddress = parent_->service->imp->insertEndpoint(localEndpoint);
		
	#ifdef MRUDP_USE_OVERLAPPED_IO
		// when you look at the following code, you will probably say to yourself
		// "WTF is this? Is this really necessary?"  The answer is: yes, yes it is.
		// Then you will ask, "does it really work?" The answer is: I hope so.
		//
		// The reason, is that on linux, when you declare SO_REUSEPORT it may give you a
		// port you *already own*!
		//
		// At first when I fixed this, I just checked with the service that I had a unique port.
		// But unfortunately, this doesn't solve the issue, because there can be multiple services running.
		// I support I could have a "meta" service which has a set of all known ports,
		// but even this will not solve the problem, because it seems that linux may give you
		// a port owned by a different process, with the same UID.  Argh!!!
		
		// We bound the port before without overlapping, so at this point the port is unique
		// move it to a temporary handle that we'll close at the last moment
		auto temporary = std::move(socket->handle);
		
		// prepare the new handle
		socket->handle.open(localEndpoint.protocol());
		socket->handle.set_option(overlap_socket(true));

		// close the old handle
		temporary.close();
		
		// and open the new one immediately
		socket->handle.bind(localEndpoint);
	#endif
	}
}

SocketImp::~SocketImp ()
{
	close();
	
	if (auto parent_ = strong(parent))
	{
		parent_->service->imp->eraseEndpoint(localEndpoint);
	}
}

#ifdef MRUDP_USE_OVERLAPPED_IO

void SocketImp::doReceive(const Address &remoteAddress_, StrongPtr<SocketNative> &socket, const StrongPtr<Packet> &receivePacket)
{
	if (!running)
		return;
		
	auto &handle = socket->handle;
	auto endpoint_ = strong<udp::endpoint>();
		
	auto *buffer_ = (char *)ptr_of(receivePacket);
	auto size = sizeof(Packet) - sizeof(Packet::dataSize);
	Address remoteAddress = remoteAddress_;

	// why am I using async_receive_from and not async_receive:
	// There is a socket A bound to port X
	// Client C sends a packet P to port X
	// There is a packet P incoming to port X
	// At that very moment we bind a new "connected" socket B to port X
	// the packet is received by socket B
	// And then we connect the "connected" socket to a remote client Y (not C)
	// at this point, when we do an async_receive, it will asign the incorrect
	// address to packet P as originating from Y, but really it came from C
	// And what's more, it seems the route is established from C -> B, and all packets
	// are received incorrectly.

	handle.async_receive_from(
		buffer(buffer_, size),
		*endpoint_,
		[this, weak_self=weak_this(this),
			remoteAddress,
			socket_=weak(socket),
			receivePacket,
		endpoint_](auto error, auto bytes_transfered) {
			if (auto self = strong(weak_self))
			{
				if (error)
				{
					xDebugLine();
				}
			
				if (!error)
				{
					auto remoteAddressReceived = toAddr(*endpoint_);
					if (remoteAddressReceived != remoteAddress)
					{
						xTraceChar(this, receivePacket->header.id, '?', (char)receivePacket->header.type);
						sLogDebug("mrudp::overlap_io", "remoteReceived isn't as expected!" << logVar(toString(remoteAddressReceived)) << " != " << logVar(toString(remoteAddress)));
					}
				
					// xTraceChar(this, 0, '_', (char)receivePacket->header.type);
					this->handleReceiveFrom(remoteAddressReceived, *receivePacket, bytes_transfered);
				}

				if (auto socket = strong(socket_))
				{
					doReceive(remoteAddress, socket, receivePacket);
				}
			}
		}
	);
}

#if 0
void SocketImp::doReceiveXX(const Address &remoteAddress_, StrongPtr<SocketNative> &socket, const StrongPtr<Packet> &receivePacket)
{
	if (!running)
		return;
		
	auto &handle = socket->handle;
		
	auto *buffer_ = (char *)ptr_of(receivePacket);
	auto size = sizeof(Packet) - sizeof(Packet::dataSize);
	Address remoteAddress = remoteAddress_;

	handle.async_receive(
		buffer(buffer_, size),
		[this, weak_self=weak_this(this),
			remoteAddress,
			socket_=weak(socket),
			receivePacket
		](auto error, auto bytes_transfered) {
			if (auto self = strong(weak_self))
			{
				if (error)
				{
					xDebugLine();
				}
			
				if (!error)
				{
					xTraceChar(this, 0, 'F', (char)receivePacket->header.type);
					this->handleReceiveFrom(remoteAddress, *receivePacket, bytes_transfered);
				}

				if (auto socket = strong(socket_))
				{
					doReceive(remoteAddress, socket, receivePacket);
				}
			}
		}
	);
}
#endif

StrongPtr<SocketNative> SocketImp::getConnectedSocket(const Address &remoteAddress)
{
	auto lock = lock_of(connectedSocketsMutex);
	
	auto &connectedSocket_ = connectedSockets[remoteAddress];
	if (auto connectedSocket = strong(connectedSocket_))
	{
		return connectedSocket;
	}
	
	if (auto parent_ = strong(parent))
	{
		auto localEndpoint = toEndpoint(parent_->getLocalAddress());
		auto remoteEndpoint = toEndpoint(remoteAddress);

		auto connectedSocket = strong<SocketNative>(*parent_->service->imp->service);
		connectedSocket->handle.open(localEndpoint.protocol());
		connectedSocket->handle.set_option(overlap_socket(true));

		connectedSocket->handle.bind(localEndpoint);
		connectedSocket->handle.connect(remoteEndpoint);
		
		connectedSocket_ = weak(connectedSocket);
		
		doReceive(remoteAddress, connectedSocket, strong<Packet>());
		
		return connectedSocket;
	}
	
	return nullptr;
}

void SocketImp::releaseConnectedSocket(const Address &)
{
	// this needs to do something and get connected eventually
}

#endif

void SocketImp::handleReceiveFrom(const Address &remoteAddress, Packet &receivePacket, size_t bytesTransferred)
{
	if (bytesTransferred >= sizeof(Header))
	{
		if (auto parent = strong(this->parent))
		{
			receivePacket.dataSize = bytesTransferred - sizeof(Header);
			parent->receive(receivePacket, remoteAddress);
		}
	}
}

void SocketImp::open()
{
	doReceiveFrom();
}

void SocketImp::connect(const Address &remoteAddress_)
{
}

void SocketImp::doReceiveFrom ()
{
	if (!running)
		return;

	auto *buffer_ = (char *)&receivePacket;
	auto size = sizeof(Packet) - sizeof(Packet::dataSize);

	socket->handle.async_receive_from(
		buffer(buffer_, size),
		remoteEndpoint,
		[weak_self=weak_this(this), this](auto error, auto bytes_transfered) {
			if (auto self = strong(weak_self))
			{
				if (error)
				{
					xDebugLine();
				}
			
				if (!error)
				{
					xTraceChar(this, 0, '$', (char)receivePacket.header.type);

					this->handleReceiveFrom(
						toAddr(remoteEndpoint),
						receivePacket,
						bytes_transfered
					);
				}

				doReceiveFrom();
			}
		}
	);

}

void SocketImp::sendViaQueue(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection)
{
	if (socket->queue.push_back(Send { addr, packet }) == 1)
		doSend(socket);
}

#ifdef MRUDP_USE_OVERLAPPED_IO

void SocketImp::send(const Address &addr, const PacketPtr &packet, Connection *connection)
{
	auto socket = connection->imp->connectedSocket;
	
#if defined(MRUDP_IO_SENDQUEUE)
	sendViaQueue(socket, addr, packet, connection);
#elif defined(MRUDP_IO_DIRECT)
	sendDirect(socket, addr, packet, connection);
#endif
}

void SocketImp::sendDirect(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection)
{
	auto &handle = socket->handle;
	
	if (!handle.is_open())
	{
		xTraceChar(this, 0, '-');
		return;
	}
	
	auto *send = new Send { addr, packet };
	
	static std::atomic<int> debugID = 0;
	auto endpoint = toEndpoint(send->address);
	auto debugID_ = debugID++;
	xLogDebug(logVar(endpoint) << logVar(debugID_) );

	char *buffer_ = (char *)ptr_of(send->packet);
	auto size = send->packet->dataSize + sizeof(Header);

	handle.async_send(
		buffer(buffer_, size),
		[this, send, debugID_, size](error_code ec, size_t bytesWritten) {

			if (ec || bytesWritten != size)
			{
				sLogDebug("mrudp::send", logOfThis(this) << "async_send failed " << logVar(ec)<< logVar(bytesWritten) << logVar(size) << logVar(debugID_));
			}
			
			delete send;
		}
	);
}

void SocketImp::doSend(const StrongPtr<SocketNative> &socket)
{
	auto *send = &socket->queue.front();
	
	auto &handle = socket->handle;
	
	if (!handle.is_open())
	{
		return;
	}

	static Atomic<int> debugID = 0;
	auto debugID_ = debugID++;
	
	char *buffer_ = (char *)ptr_of(send->packet);
	auto size = send->packet->dataSize + sizeof(Header);
	
	handle.async_send(
		buffer(buffer_, size),
		[this, socket, buffer_, debugID_, size](error_code ec, size_t bytesWritten) {

			if (ec || bytesWritten != size)
			{
				sLogDebug("mrudp::send", logOfThis(this) << "async_send failed " << logVar(ec)<< logVar(bytesWritten) << logVar(size) << logVar(debugID_));
			}
		
			if (socket->queue.pop_front())
				doSend(socket);
		}
	);
}

#else // !MRUDP_USE_OVERLAPPED_IO

void SocketImp::send(const Address &addr, const PacketPtr &packet, Connection *connection)
{
#if defined(MRUDP_IO_SENDQUEUE)
	sendViaQueue(socket, addr, packet, connection);
#elif defined(MRUDP_IO_DIRECT)
	sendDirect(socket, addr, packet, connection);
#endif
}

void SocketImp::sendDirect(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection)
{
	auto &handle = socket->handle;
	
	if (!handle.is_open())
	{
		xTraceChar(this, 0, '-');
		return;
	}
	
	auto *send = new Send { addr, packet };
	
	static std::atomic<int> debugID = 0;
	auto endpoint = toEndpoint(send->address);
	auto debugID_ = debugID++;
	xLogDebug(logVar(endpoint) << logVar(debugID_) );

	char *buffer_ = (char *)ptr_of(send->packet);
	auto size = send->packet->dataSize + sizeof(Header);

	handle.async_send_to(
		buffer(buffer_, size),
		endpoint,
		[this, send, debugID_, size](error_code ec, size_t bytesWritten) {

			if (ec || bytesWritten != size)
			{
				sLogDebug("mrudp::send", logOfThis(this) << "async_send failed " << logVar(ec)<< logVar(bytesWritten) << logVar(size) << logVar(debugID_));
			}
			
			delete send;
		}
	);
}

void SocketImp::doSend(const StrongPtr<SocketNative> &socket)
{
	auto *send = &socket->queue.front();
	
	auto &handle = socket->handle;
	
	if (!handle.is_open())
	{
		xTraceChar(this, 0, '-');
		return;
	}
	
	static std::atomic<int> debugID = 0;
	auto endpoint = toEndpoint(send->address);
	auto debugID_ = debugID++;
	xLogDebug(logVar(endpoint) << logVar(debugID_) );

	char *buffer_ = (char *)ptr_of(send->packet);
	auto size = send->packet->dataSize + sizeof(Header);

	handle.async_send_to(
		buffer(buffer_, size),
		endpoint,
		[this, socket, buffer_, debugID_, size](error_code ec, size_t bytesWritten) {

			if (ec || bytesWritten != size)
			{
				sLogDebug("mrudp::send", logOfThis(this) << "async_send failed " << logVar(ec)<< logVar(bytesWritten) << logVar(size) << logVar(debugID_));
			}
		
			if (socket->queue.pop_front())
				doSend(socket);
		}
	);
}

#endif // +-MRUDP_USE_OVERLAPPED_IO

void SocketImp::close ()
{
	if (running)
	{
		running = false;
		xLogDebug(logOfThis(this) << socket->handle.local_endpoint() << " handle is closing " << socket->handle.native_handle());

//		handle.cancel();
		socket->handle.close();
		
	}
}

mrudp_addr_t SocketImp::getLocalAddress ()
{
	boost::system::error_code ec;
	auto endpoint = socket->handle.local_endpoint(ec);
	return toAddr(endpoint);
}

} // namespace
} // namespace
} // namespace
