#pragma once

#include "Types.h"
#include "Packet.h"
#include "socket/Drop.h"
#include "proxy/Transformer.h"

namespace timprepscius {
namespace mrudp {

struct Service;
struct Connection;

namespace imp {
struct SocketImp;
}

// --------------------------------------------------------
// Socket
//
// Socket contains the look ups for the connections
// and contains the callback data
// --------------------------------------------------------
struct Socket : StrongThis<Socket>
{
	Socket (const StrongPtr<Service> &system);
	~Socket ();
	
	StrongPtr<Service> service;
	StrongPtr<imp::SocketImp> imp;
	
	void open (const Address &address);

	bool closeRequested = false;
	void close ();
	void closeUser ();
	
	proxy::Transformer proxy;
	Set<ShortConnectionID> shortConnectionIDs;
	ShortConnectionID generateShortConnectionID ();
	bool isFull();
	
	LongConnectionID generateLongConnectionID ();
	
	Mutex userDataMutex;
	Atomic<bool> userDataDisposed = true;

	void *userData = nullptr;
	mrudp_should_accept_callback shouldAccept;
	mrudp_accept_callback acceptHandler;
	mrudp_close_callback closeHandler;
	
	struct LookUp
	{
		ShortConnectionID shortID = 0;
		LongConnectionID longID = NullLongConnectionID;
	} ;
	
	RecursiveMutex connectionsMutex;
	UnorderedMap<LongConnectionID, StrongPtr<Connection>> connections;
	UnorderedMap<ShortConnectionID, StrongPtr<Connection>> receiveConnections;
	
	LookUp getLookUp(Packet &packet);
	StrongPtr<Connection> findConnection(const LookUp &lookup, Packet &packet, const Address &remoteAddress, const ProxyID &proxyID);
	StrongPtr<Connection> generateConnection(const LookUp &lookup, Packet &packet, const Address &remoteAddress, const ProxyID &proxyID);
	StrongPtr<Connection> findOrGenerateConnection(const LookUp &lookup, Packet &packet, const Address &remoteAddress, const ProxyID &proxyID);
	
	void insert(const StrongPtr<Connection> &connection);
	void erase(Connection *connection);
	StrongPtr<Connection> getConnection();

	Drop drop;
	void send(const PacketPtr &packet, Connection *connection, const Address *to);
	void receive(Packet &packet, const Address &from);
	
	void listen(
		void *userData,
		mrudp_should_accept_callback &&shouldAcceptCallback,
		mrudp_accept_callback &&acceptCallback,
		mrudp_close_callback &&closeCallback
	);
	
	StrongPtr<Connection> connect(
		const Address &address,
		const ConnectionOptions *options,
		void *userData,
		mrudp_receive_callback &&receiveCallback,
		mrudp_close_callback &&eventCallback
	);
	
	Address getLocalAddress();
};

// --------------------------------------------------------
// handles
// --------------------------------------------------------

typedef mrudp_socket_t SocketHandle;

SocketHandle newHandle(const StrongPtr<Socket> &socket);
StrongPtr<Socket> toNative(SocketHandle handle);
void deleteHandle (SocketHandle handle);
StrongPtr<Socket> closeHandle (SocketHandle handle_);


} // namespace
} // namespace
