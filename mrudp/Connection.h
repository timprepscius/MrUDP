#pragma once

#include "sender/Sender.h"
#include "receiver/Receiver.h"
#include "connection/Probe.h"
#include "Crypto.h"
#include "Handshake.h"
#include "Statistics.h"
#include "NetworkPath.h"

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
	ShortConnectionID remoteID = 0, localID = 0;
	
	// the remote address
	mrudp_addr_t remoteAddress; // Peer address

	// the options for how the connectino functions
	ConnectionOptions options;

	// connection mechanisms
	Handshake handshake;
	NetworkPath networkPath;
	Sender sender;
	Receiver receiver;
	Probe probe;
	ConnectionStatistics statistics;
	
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
		ShortConnectionID localID
	);
	~Connection ();

	ErrorCode send(const char *buffer, int size, Reliability reliable);

	void openUser(const ConnectionOptions *options, void *userData_, mrudp_receive_callback_fn receiveHandler_, mrudp_close_callback_fn closeHandler_);
	void closeUser ();

	void open ();

	bool canSend ();
	void send(const PacketPtr &packet, Address *address=nullptr);
	void resend(const PacketPtr &packet, Address *address=nullptr);
	void send_(const PacketPtr &packet, Address *address);
	
	void receive(Packet &p, const Address &remoteAddress);
	void receive(char *buffer, int size, Reliability reliable);
	
	void possiblyClose ();
	void close ();
	
	// could end up being an alias to close(event)
	void fail(mrudp_event_t event);
	void close(mrudp_event_t event);
	
	void finishWhenReady ();
	void finish ();
	
	void onRemoteAddressChanged (const Address &remoteAddress);

#ifdef MRUDP_ENABLE_DEBUG_HOOK
	void __debugHook ();
#endif
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
