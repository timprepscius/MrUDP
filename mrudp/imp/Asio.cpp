#include "Asio.h"

#include "../Base.h"
#include "../Types.h"
#include <boost/asio.hpp>
#include <iostream>

#include "AsioTesting.h"

namespace timprepscius {
namespace mrudp {
namespace imp {

// --------------------------

ConnectionOptions getDefaultConnectionOptions ()
{
	return ConnectionOptions {
		.coalesce_reliable = {
			.mode = MRUDP_COALESCE_STREAM,
			.delay_ms = 5
		},

		.coalesce_unreliable = {
			.mode = MRUDP_COALESCE_PACKET,
			.delay_ms = 5
		}
	} ;
}

#if defined(SYS_LINUX)
OptionsImp systemDefaultOptions {
	.connection = getDefaultConnectionOptions(),

	.overlapped_io = 0,
	.send_via_queue = 1,
	.thread_quantity = 1,
} ;
#else
OptionsImp systemDefaultOptions {
	.connection = getDefaultConnectionOptions(),

	.overlapped_io = 1,
	.send_via_queue = 0,
	.thread_quantity = int8_t(std::thread::hardware_concurrency() - 1),
} ;
#endif

OptionsImp getDefaultOptions()
{
	return systemDefaultOptions;
}

void merge(OptionsImp &lhs, const OptionsImp &rhs)
{
	lhs.connection = mrudp::merge(lhs.connection, rhs.connection);

	if (lhs.overlapped_io == -1)
		lhs.overlapped_io = rhs.overlapped_io;
		
	if (lhs.send_via_queue == -1)
		lhs.send_via_queue = rhs.send_via_queue;

	if (lhs.thread_quantity == -1)
		lhs.thread_quantity = rhs.thread_quantity;
}

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

ServiceImp::ServiceImp (Service *parent_, const OptionsImp *options_) :
	parent(weak_this(parent_)),
	options(options_ ? *options_ : systemDefaultOptions)
{
	merge(options, systemDefaultOptions);

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
		
		auto processor_count =
			options.thread_quantity ?
				options.thread_quantity :
				std::thread::hardware_concurrency();
		
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
	
}

ConnectionImp::~ConnectionImp()
{
	xLogDebug(logOfThis(this));
	stop();
}

mrudp_error_code_t ConnectionImp::open ()
{
	auto parent_ = strong(parent);

	if (!parent_)
		return MRUDP_ERROR_GENERAL_FAILURE;
		
	if (parent_->socket->imp->options.overlapped_io)
	{
		connectedSocket = parent_->socket->imp->getConnectedSocket(parent_->remoteAddress);
		
		if (!connectedSocket)
			return MRUDP_ERROR_GENERAL_FAILURE;
	}
		
	return MRUDP_OK;
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

void ConnectionImp::relocate ()
{
	reacquireConnectedSocket();
}

void ConnectionImp::onRemoteAddressChanged ()
{
	reacquireConnectedSocket();
}

void ConnectionImp::reacquireConnectedSocket ()
{
	auto parent_ = strong(parent);
	debug_assert(parent_);
	
	connectedSocket = nullptr;
	if (parent_->socket->imp->options.overlapped_io)
		connectedSocket = parent_->socket->imp->getConnectedSocket(parent_->remoteAddress);
}

// -------------------------------------

void SocketNative::send(const Send &send, Function<void (const error_code &)> &&f)
{
	auto lock = shared_lock_of(handleMutex);
	
	if (!handle.is_open())
	{
		return;
	}
	
	static Atomic<int> debugID = 0;
	auto debugID_ = debugID++;
	
	char *buffer_ = (char *)ptr_of(send.packet);
	auto size = send.packet->dataSize + sizeof(Header);
	
	if (isConnected)
	{
		handle.async_send(
			buffer(buffer_, size),
			[this, buffer_, debugID_, size, f](error_code ec, size_t bytesWritten) {

				if (ec || bytesWritten != size)
				{
					sLogDebug("mrudp::send", logOfThis(this) << "async_send failed " << logVar(ec)<< logVar(bytesWritten) << logVar(size) << logVar(debugID_));
				}

				f(ec);
			}
		);
	}
	else
	{
		auto endpoint = toEndpoint(send.address);

		handle.async_send_to(
			buffer(buffer_, size),
			endpoint,
			[this, send, debugID_, size, f](error_code ec, size_t bytesWritten) {

				if (ec || bytesWritten != size)
				{
					sLogDebug("mrudp::send", logOfThis(this) << "async_send failed " << logVar(ec)<< logVar(bytesWritten) << logVar(size) << logVar(debugID_));
				}
				
				f(ec);
			}
		);
	}
}

void SocketNative::receive(Receive &receive, Function<void (const error_code &)> &&f)
{
	auto lock = shared_lock_of(handleMutex);
	
	if (!handle.is_open())
	{
		return;
	}

	auto *buffer_ = (char *)&receive.packet;
	auto size = sizeof(Packet) - sizeof(Packet::dataSize);
	
	if (isConnected)
	{
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
		
		auto remoteEndpoint = this->remoteEndpoint;

		handle.async_receive_from(
			buffer(buffer_, size),
			receive.endpoint,
			[this, remoteEndpoint, &receive, f=std::move(f)](auto error, auto bytesTransferred) {
				if (error)
				{
					xDebugLine();
				}
				
				if (!error)
				{
					if (receive.endpoint != remoteEndpoint)
					{
						xTraceChar(this, receive.packet.header.id, '?', (char)receive.packet.header.type);
						sLogDebug("mrudp::overlap_io", "remoteReceived isn't as expected!" << logVar(receive.endpoint) << " != " << logVar(remoteEndpoint));
					}
					
					if (bytesTransferred < sizeof(Header))
						error = errc::make_error_code(errc::illegal_byte_sequence);
					else
						receive.packet.dataSize = bytesTransferred - sizeof(Header);
				}

				f(error);
			}
		);
	}
	else
	{
		handle.async_receive_from(
			buffer(buffer_, size),
			receive.endpoint,
			[&receive, f=std::move(f)](auto error, auto bytesTransferred) {
				if (error)
				{
					xDebugLine();
				}
				
				if (!error)
				{
					if (bytesTransferred < sizeof(Header))
						error = errc::make_error_code(errc::illegal_byte_sequence);
					else
						receive.packet.dataSize = bytesTransferred - sizeof(Header);
				}

				f(error);
			}
		);
	}

}

// -------------------------------------

SocketImp::SocketImp(const StrongPtr<Socket> &parent_, const Address &address) :
	parent(weak(parent_)),
	socket(strong<SocketNative>(*parent_->service->imp->service)),
	options(parent_->service->imp->options)
{
	acquireAddress(address);
}

void SocketImp::acquireAddress(const Address &address)
{
	auto parent_ = strong(parent);
	debug_assert(parent_);
	
	bool acquiredAddress = false;
	while (!acquiredAddress)
	{
		boost::system::error_code error;

		auto handleError = [&](auto &error) {
			sLogDebug("debug", logVar(this) << logVar(localEndpoint) << logVar(error.message()));
		};

		socket->handle.close(error);
		MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__socket_handle_close, error);
		
		if (error)
			return handleError(error);
		
		auto endpoint = toEndpoint(address);
		socket->handle.open(endpoint.protocol());
		MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__socket_handle_open, error);

		if (error)
			return handleError(error);

		socket->handle.bind(endpoint);
		MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__socket_handle_bind, error);

		if (error)
			return handleError(error);

		localEndpoint = socket->handle.local_endpoint(error);

		if (error)
			return handleError(error);
		
		acquiredAddress = parent_->service->imp->insertEndpoint(localEndpoint);
		
		if (options.overlapped_io == 1)
		{
			// when you look at the following code, you will probably say to yourself
			// "WTF is this? Is this really necessary?"  The answer is: yes, yes it is.
			// Then you will ask, "does it really work?" The answer is: I hope so.
			//
			// The reason, is that on linux, when you declare SO_REUSEPORT it may give you a
			// port you *already own*!
			//
			// At first when I fixed this, I just checked with the service that I had a unique port.
			// But unfortunately, this doesn't solve the issue, because there can be multiple services running.
			// I suppose I could have a "meta" service which has a set of all known ports,
			// but even this will not solve the problem, because it seems that linux may give you
			// a port owned by a different process, with the same UID.  Argh!!!
			
			// We bind the port before without overlapping, so at this point the port is unique
			// move it to a temporary handle that we'll close at the last moment
			// it would be great if I locked a system wide mutex
			auto temporary = std::move(socket->handle);
			
			// prepare the new handle
			socket->handle.open(localEndpoint.protocol());
			MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__socket_handle_open_overlapped, error);
			
			if (error)
				return handleError(error);

			socket->handle.set_option(overlap_socket(true), error);
			MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__socket_handle_set_option_overlapped, error);
			
			if (error)
				return handleError(error);

			// close the old handle
			temporary.close();
			MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__temporary_close_overlapped, error);
			
			if (error)
				return handleError(error);
			
			// and open the new one immediately
			sLogDebug("debug", logVar(this) << logVar(localEndpoint));
			
			socket->handle.bind(localEndpoint, error);
			MRUDP_ASIO_TEST_GENERATE_FAILURE(acquireAddress__socket_handle_bind_overlapped, error);

			if (error)
				return handleError(error);
		}
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

		auto handleError = [&](auto &error) {
			sLogDebug("debug", logVar(this) << logVar(localEndpoint) << logVar(remoteEndpoint) << logVar(error.message()));
		};

		boost::system::error_code error;

		sLogDebug("debug", logVar(this) << logVar(localEndpoint) << logVar(remoteEndpoint));

		auto connectedSocket = strong<SocketNative>(*parent_->service->imp->service, remoteEndpoint);

		connectedSocket->handle.open(localEndpoint.protocol(), error);
		MRUDP_ASIO_TEST_GENERATE_FAILURE(getConnectedSocket__connectedSocket_handle_open, error);

		if (error)
		{
			handleError(error);
			return nullptr;
		}
		
		connectedSocket->handle.set_option(overlap_socket(true), error);
		MRUDP_ASIO_TEST_GENERATE_FAILURE(getConnectedSocket__connectedSocket_handle_set_option, error);
		
		if (error)
		{
			handleError(error);
			return nullptr;
		}

		connectedSocket->handle.bind(localEndpoint, error);
		MRUDP_ASIO_TEST_GENERATE_FAILURE(getConnectedSocket__connectedSocket_handle_bind, error);

		if (error)
		{
			handleError(error);
			return nullptr;
		}
		
		connectedSocket->handle.connect(remoteEndpoint, error);
		if (error)
		{
			handleError(error);
			return nullptr;
		}
		
		connectedSocket_ = weak(connectedSocket);
		
		doReceive(connectedSocket, strong<Receive>());
		
		return connectedSocket;
	}
	
	return nullptr;
}

void SocketImp::releaseConnectedSocket(const Address &)
{
	// this needs to do something and get connected eventually
}

void SocketImp::handleReceiveFrom(const Address &remoteAddress, Packet &receivePacket)
{
	if (auto parent = strong(this->parent))
	{
		parent->receive(receivePacket, remoteAddress);
	}
}

void SocketImp::open()
{
	debug_assert(running == false);
	running = true;
	
	doReceive(socket, strong<Receive>());
}

void SocketImp::doReceive(const StrongPtr<SocketNative> &socket, const StrongPtr<Receive> &receive)
{
	if (!running)
		return;
		
	socket->receive(
		*receive,
		[this, weak_self=weak_this(this), socket_=weak(socket), receive](auto &error)
		{
			if (auto self = strong(weak_self))
			{
				if (!error)
				{
					this->handleReceiveFrom(toAddr(receive->endpoint), receive->packet);
				}
				
				if (auto socket = strong(socket_))
				{
					doReceive(socket, receive);
				}
			}
		}
	);
}

mrudp_error_code_t SocketImp::send(const PacketPtr &packet, Connection *connection, const Address *address_)
{
	auto *socket = &this->socket;
	
	if (options.overlapped_io == 1 && !address_)
		socket = &connection->imp->connectedSocket;

	if (!*socket)
		return MRUDP_ERROR_GENERAL_FAILURE;

	auto *address = address_ ? address_ : &connection->remoteAddress;
	
	
	if (options.send_via_queue == 1)
		sendViaQueue(*socket, *address, packet, connection);
	else
		sendDirect(*socket, *address, packet, connection);
		
	return MRUDP_OK;
}

void SocketImp::sendDirect(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection)
{
	Send send { addr, packet };
	socket->send(send, [packet](auto &error) {});
}

void SocketImp::sendViaQueue(const StrongPtr<SocketNative> &socket, const Address &addr, const PacketPtr &packet, Connection *connection)
{
	if (socket->queue.push_back(Send { addr, packet }) == 1)
		doSend(socket);
}

void SocketImp::doSend(const StrongPtr<SocketNative> &socket)
{
	auto *send = &socket->queue.front();

	socket->send(*send, [this, socket](auto &error) {
		if (socket->queue.pop_front() > 0)
			doSend(socket);
	});
}

void SocketImp::close ()
{
	if (running)
	{
		running = false;
		xLogDebug(logOfThis(this) << socket->handle.local_endpoint() << " handle is closing " << socket->handle.native_handle());

		boost::system::error_code error;
		auto handleError = [&](auto &error) {
			sLogDebug("debug", logVar(this) << logVar(error.message()));
		};

		{
			auto lock = lock_of(socket->handleMutex);
			socket->handle.close(error);

			if (error)
				handleError(error);
		}
		
		auto lock = lock_of(connectedSocketsMutex);
		for (auto &[remote, socket_] : connectedSockets)
		{
			if (auto socket = strong(socket_))
			{
				boost::system::error_code error;

				auto lock = lock_of(socket->handleMutex);
				socket->handle.close(error);

				if (error)
					handleError(error);
			}
		}
	}
}

void SocketImp::relocate(const Address &address)
{
	auto parent_ = strong(parent);
	
	auto lock = lock_of(parent_->connectionsMutex);
	
	close();
	acquireAddress(address);
	open();
	
	for (auto &[id, connection] : parent_->connections)
	{
		connection->imp->relocate();
	}
}

mrudp_addr_t SocketImp::getLocalAddress ()
{
	auto lock = shared_lock_of(socket->handleMutex);

	boost::system::error_code ec;
	auto endpoint = socket->handle.local_endpoint(ec);
	return toAddr(endpoint);
}

} // namespace
} // namespace
} // namespace
