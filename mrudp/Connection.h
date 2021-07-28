#pragma once

#include "sender/Sender.h"
#include "receiver/Receiver.h"
#include "connection/Probe.h"
#include "Crypto.h"

namespace timprepscius {
namespace mrudp {

struct Socket;

namespace imp {
struct ConnectionImp;
}

// --------------------------------------------------------
// Connection
//
// Connection contains the Sender, the Receiver, the Probe
// and various state.
// --------------------------------------------------------

struct Connection : StrongThis<Connection>
{
	StrongPtr<imp::ConnectionImp> imp;
	StrongPtr<Socket> socket = nullptr;

	// the long lookup id for this connection
	LongConnectionID id;
	
	// the short lookup ids for remote and local.
	ShortConnectionID remoteID, localID = 0;
	
	// the remote address
	mrudp_addr_t remoteAddress; // Peer address

	// connection mechanisms
	Sender sender;
	Receiver receiver;
	Probe probe;
	
#ifdef MRUDP_ENABLE_CRYPTO
	StrongPtr<ConnectionCrypto> crypto;
#endif

	// data for callbacks
	Mutex userDataMutex;
	void *userData = nullptr;
	mrudp_receive_callback_fn receiveHandler = nullptr;
	mrudp_close_callback_fn closeHandler = nullptr;
	mrudp_event_t closeReason = MRUDP_EVENT_CLOSED;

	// state data
	Atomic<bool> closed = false;

	Connection(
		const StrongPtr<Socket> &socket_,
		LongConnectionID id,
		const mrudp_addr_t &remoteAddress_,
		ShortConnectionID localID,
		ShortConnectionID remoteID
	);
	~Connection ();

	void setHandlers(void *userData_, mrudp_receive_callback_fn receiveHandler_, mrudp_close_callback_fn closeHandler_);

	void open ();

	bool canSend ();
	void send(const char *buffer, int size, int reliable);
	void send_(const PacketPtr &packet);
	void send(const PacketPtr &packet);
	void resend(const PacketPtr &packet);
	
	void receive(Packet &p);
	
	void possiblyClose ();
	void close ();
	
	// could end up being an alias to close(event)
	void fail(mrudp_event_t event);
	void close(mrudp_event_t event);
	
	void finishWhenReady ();
	void finish ();
};

// --------------------------------------------------------
// handles
// --------------------------------------------------------

typedef mrudp_connection_t ConnectionHandle;

ConnectionHandle newHandle(const StrongPtr<Connection> &connection);
StrongPtr<Connection> toNative(ConnectionHandle handle);
StrongPtr<Connection> closeHandle (ConnectionHandle handle);
void deleteHandle (ConnectionHandle handle);


} // namespace
} // namespace
