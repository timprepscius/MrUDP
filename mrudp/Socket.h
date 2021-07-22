#pragma once

#include "Types.h"
#include "Packet.h"
#include "socket/Drop.h"

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
	
	void open (const Address &address);
	
	StrongPtr<Service> service;
	StrongPtr<imp::SocketImp> imp;
	
	bool closeRequested = false;
	
	LongConnectionID generateLongConnectionID ();
	Atomic<ShortConnectionID> nextConnectionID = 1;
	
	Mutex userDataMutex;
	void *userData = nullptr;
	mrudp_accept_callback_fn acceptHandler = nullptr;
	mrudp_close_callback_fn closeHandler = nullptr;
	
	struct LookUp
	{
		ShortConnectionID shortID = 0;
		LongConnectionID longID = 0;
	} ;
	
	RecursiveMutex connectionsMutex;
	UnorderedMap<LongConnectionID, StrongPtr<Connection>> connections;
	UnorderedMap<ShortConnectionID, StrongPtr<Connection>> receiveConnections;
	
	LookUp getLookUp(Packet &packet);
	StrongPtr<Connection> findConnection(const LookUp &lookup, Packet &packet, const mrudp_addr_t &remoteAddress);
	StrongPtr<Connection> generateConnection(const LookUp &lookup, Packet &packet, const mrudp_addr_t &remoteAddress);
	StrongPtr<Connection> findOrGenerateConnection(const LookUp &lookup, Packet &packet, const mrudp_addr_t &remoteAddress);
	
	void insert(const StrongPtr<Connection> &connection);
	void erase(Connection *connection);
	StrongPtr<Connection> getConnection();

	Drop drop;
	void send(const PacketPtr &packet, const Address &to, Connection *connection);
	void receive(Packet &packet, const Address &from);
	
	void listen(void *userData, mrudp_accept_callback_fn acceptCallback, mrudp_close_callback_fn closeCallback);
	
	StrongPtr<Connection> connect(
		const Address &address, void *userData,
		mrudp_receive_callback_fn receiveCallback,
		mrudp_close_callback_fn eventCallback
	);
	
	void close ();

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
