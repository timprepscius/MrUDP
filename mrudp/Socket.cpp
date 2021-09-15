#include "Socket.h"
#include "Connection.h"
#include "Service.h"
#include "Implementation.h"

namespace timprepscius {
namespace mrudp {

Socket::Socket(const StrongPtr<Service> &service_) :
	service(service_)
{
	xLogDebug(logOfThis(this));
}

Socket::~Socket ()
{
	imp->close();
	imp = nullptr;
	
	if (closeHandler)
		closeHandler(userData, MRUDP_EVENT_CLOSED);
}

void Socket::open (const Address &address)
{
	imp = strong_thread(strong<imp::SocketImp>(strong_this(this), address));
	imp->open();
}

bool Socket::isFull()
{
	return shortConnectionIDs.size() == std::numeric_limits<ShortConnectionID>().max()-1;
}

ShortConnectionID Socket::generateShortConnectionID()
{
	ShortConnectionID id = service->random.next<ShortConnectionID>();

	auto lock = lock_of(connectionsMutex);
	while (id == 0 || !shortConnectionIDs.insert(id).second)
	{
		id++;
	}
	
	return id;
}

LongConnectionID Socket::generateLongConnectionID()
{
	return service->random.next<LongConnectionID>();
}

void Socket::listen(void *userData_, mrudp_accept_callback_fn acceptHandler_, mrudp_close_callback_fn closeHandler_)
{
	userData = userData_;
	acceptHandler = acceptHandler_;
	closeHandler = closeHandler_;
}

StrongPtr<Connection> Socket::connect(
	const Address &remoteAddress,
	const ConnectionOptions *options,
	void *userData,
	mrudp_receive_callback_fn receiveHandler_,
	mrudp_close_callback_fn closeHandler_
)
{
	auto connection = strong<Connection>(
		strong_this(this),
		generateLongConnectionID(),
		remoteAddress,
		generateShortConnectionID()
	);
	
	connection->openUser(options, userData, receiveHandler_, closeHandler_);

	insert(connection);
	connection->open();
	connection->handshake.initiate();

	sLogDebugIf(connection->imp->connectedSocket, "mrudp::overlap_io", "connect " << logVar(uint64_t(connection->id)) << logVar(ptr_of(connection)) << logVar(this) << logVar(toString(connection->remoteAddress)) << logVar(toString(getLocalAddress())) << logVar(connection->imp->connectedSocket->handle.local_endpoint()));

	return connection;
}

void Socket::insert(const StrongPtr<Connection> &connection)
{
	auto lock = lock_of(connectionsMutex);
	connections[connection->id] = connection;
	receiveConnections[connection->localID] = connection;
}

void Socket::erase(Connection *connection)
{
	auto lock = lock_of(connectionsMutex);
	
	{
		auto i = receiveConnections.find(connection->localID);
		if (i != receiveConnections.end())
		{
			receiveConnections.erase(i);
		}
	}

	{
		auto i = connections.find(connection->id);
		if (i != connections.end())
		{
			sLogDebug("mrudp::overlap_io", "erase " << logVar(uint64_t(connection->id)) << logVar(connection) << logVar(this) << logVar(toString(connection->remoteAddress)) << logVar(toString(getLocalAddress())));
		
			connections.erase(i);
		}
	}
	
	{
		auto i = shortConnectionIDs.find(connection->localID);
		if (i != shortConnectionIDs.end())
		{
			shortConnectionIDs.erase(i);
		}
	}
}

void Socket::close ()
{
	auto lock = lock_of(userDataMutex);
	userData = nullptr;
	acceptHandler = nullptr;
	closeHandler = nullptr;
}

void Socket::send(const PacketPtr &packet, Connection *connection, const Address *to)
{
	xLogDebug(logOfThis(this) << logLabelVar("local", toString(getLocalAddress())) << logLabelVar("remote", (to ? toString(*to) : String())) << logVar(packet->header.connection) << logVar((char)packet->header.type) << logVar(packet->header.id));

	if (drop.shouldDrop())
	{
		xLogDebug(logOfThis(this) << logLabelVar("local", toString(getLocalAddress())) << logLabelVar("remote", (to ? toString(*to) : String())) << logVar(packet->header.connection) << logVar((char)packet->header.type) << logVar(packet->header.id) << logLabel("DROPPING INTENTIONALLY"));
	}
	
	imp->send(packet, connection, to);
}

Socket::LookUp Socket::getLookUp(Packet &packet)
{
	LookUp lookup;
	lookup.shortID = packet.header.connection;
	
	if (lookup.shortID == 0)
		popData(packet, lookup.longID);
		
	return lookup;
}


StrongPtr<Connection> Socket::findConnection(const LookUp &lookup, Packet &packet, const mrudp_addr_t &remoteAddress)
{
	if (lookup.longID != 0)
	{
		auto connection_ = connections.find(lookup.longID);
		if (connection_ != connections.end())
			return connection_->second;
		
		return nullptr;
	}

	if (lookup.shortID != 0)
	{
		auto connection_ = receiveConnections.find(lookup.shortID);
		if (connection_ != receiveConnections.end())
			return connection_->second;
	}
		
	return nullptr;
}

StrongPtr<Connection> Socket::generateConnection(const LookUp &lookup, Packet &packet, const Address &remoteAddress)
{
	xLogDebug(logOfThis(this) << logLabelVar("local", toString(getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << "new connection");
	
	sLogDebug("mrudp::overlap_io", "G " << logVar(uint64_t(lookup.longID)) << logVar(this) << logVar(toString(remoteAddress)) << logVar(toString(getLocalAddress())) << logVar((char)packet.header.type));
	
	if (isFull())
	{
		return nullptr;
	}
	
	if (packet.header.type != H0)
	{
		sLogDebug("mrudp::overlap_io", "ERROR " << logVar((char)packet.header.type) << logVar(uint64_t(lookup.longID)) << logVar(this) << logVar(toString(remoteAddress)) << logVar(toString(getLocalAddress())));

		xTraceChar(this, packet.header.id, '!', '0');
		xLogDebug(logOfThis(this) << logLabelVar("local", toString(getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << "not MRUDP H0");

		// @TODO: throw exception/ return some value?
		return nullptr;
	}
	
	if (lookup.longID == 0)
	{
		sLogDebug("mrudp::overlap_io", "ERROR NO LONG " << logVar((char)packet.header.type) << logVar(uint64_t(lookup.longID)) << logVar(this) << logVar(toString(remoteAddress)) << logVar(toString(getLocalAddress())));

		xTraceChar(this, packet.header.id, '!', 'L');

		xLogDebug(logOfThis(this) << logLabelVar("local", toString(getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << "no LONGID");

		return nullptr;
	}

	auto lock = lock_of(userDataMutex);
	if (!acceptHandler)
	{
		sLogDebug("mrudp::overlap_io", "ERROR NO ACCEPT " << logVar((char)packet.header.type) << logVar(uint64_t(lookup.longID)) << logVar(this) << logVar(toString(remoteAddress)) << logVar(toString(getLocalAddress())));

		// @TODO: throw exception/ return some value?
		return nullptr;
	}
	
	auto localID = generateShortConnectionID();
	auto connection = strong<Connection>(strong_this(this), lookup.longID, remoteAddress, localID);
	
	insert(connection);
	connection->open();
		
	auto connectionHandle = newHandle(connection);
	
	int status;
	if (mrudp_failed(status = acceptHandler(userData, (mrudp_connection_t)connectionHandle)))
	{
		sLogDebug("mrudp::overlap_io", "ERROR NOT ACCEPTED " << logVar((char)packet.header.type) << logVar(uint64_t(lookup.longID)) << logVar(this) << logVar(toString(remoteAddress)) << logVar(toString(getLocalAddress())));

		// do something
		xLogDebug(logOfThis(this) << "not accepted");
		connection->close(MRUDP_EVENT_NOT_ACCEPTED);
		deleteHandle(connectionHandle);

		return nullptr;
	}
	
	return connection;
}

StrongPtr<Connection> Socket::findOrGenerateConnection(const LookUp &lookup, Packet &packet, const Address &remoteAddress)
{
	auto lock = lock_of(connectionsMutex);
	auto connection = findConnection(lookup, packet, remoteAddress);

	if(connection == nullptr)
	{
		connection = generateConnection(lookup, packet, remoteAddress);
	}

	return connection;
}


void Socket::receive(Packet &packet, const Address &remoteAddress)
{
	xLogDebug(logOfThis(this) << logLabel("begin") << logLabelVar("local", toString(getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVar(packet.header.connection) << logVar((char)packet.header.type) << logVar(packet.header.id));

	auto lookup = getLookUp(packet);
	
	if (auto connection = findOrGenerateConnection(lookup, packet, remoteAddress))
	{
		connection->receive(packet, remoteAddress);
	}
	
	xLogDebug(logOfThis(this) << "end");
}

Address Socket::getLocalAddress()
{
	return imp->getLocalAddress();
}


// ---------------

SocketHandle newHandle(const StrongPtr<Socket> &socket)
{
	return (mrudp_socket_t) new StrongPtr<Socket>(socket);
}

StrongPtr<Socket> toNative(SocketHandle handle_)
{
	if (!handle_)
		return nullptr;

	auto handle = (StrongPtr<Socket> *)handle_;
	return (*handle);
}

StrongPtr<Socket> closeHandle (SocketHandle handle_)
{
	if (!handle_)
		return nullptr;
		
	StrongPtr<Socket> result;
	auto handle = (StrongPtr<Socket> *)handle_;
	std::swap(result, *handle);
	
	return result;
}

void deleteHandle (SocketHandle handle_)
{
	auto handle = (StrongPtr<Socket> *)handle_;
	delete handle;
}

} // namespace
} // namespace
