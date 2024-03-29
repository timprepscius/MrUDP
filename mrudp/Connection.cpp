#include "Connection.h"
#include "Implementation.h"

#include <iostream>

namespace timprepscius {
namespace mrudp {

#ifdef LOG_DEBUG
	static std::atomic<size_t> NumConnection = 0;
#endif

Connection::Connection(const StrongPtr<Socket> &socket_, LongConnectionID id_, const Address &remoteAddress_, ShortConnectionID localID_) :
	socket(socket_),
	id(id_),
	localID(localID_),
	remoteAddress(remoteAddress_),
	sender(this),
	receiver(this),
	probe(this),
	handshake(this),
	handshake_options(this),
	networkPath(this),
	options(socket->service->imp->options.connection)
{
	#ifdef MRUDP_ENABLE_CRYPTO
		crypto = strong<ConnectionCrypto>(socket->service->crypto);
	#endif

	#ifdef LOG_DEBUG
		++NumConnection;
		sLogDebug("mrudp::opens", logOfThis(this) << NumConnection);
	#endif
	sLogDebug("mrudp::life_cycle", logOfThis(this) << NumConnection);
	xLogDebug(logOfThis(this) << logLabel("socket connection") << logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)));
}

Connection::~Connection ()
{
	xTraceChar(this, 0, '~');
	
	#ifdef LOG_DEBUG
		--NumConnection;
	#endif

	xLogDebug(logOfThis(this));
	sLogDebug("mrudp::life_cycle", logOfThis(this) << NumConnection);
	sLogDebug("com::closes", logOfThis(this) << NumConnection)
	
	imp = nullptr;
}

void Connection::openUser(const ConnectionOptions *options_, void *userData_, mrudp_receive_callback &&receiveHandler_, mrudp_close_callback &&closeHandler_)
{
	if (options_)
		options = merge(*options_, options);

	auto expected = true;
	if (userDataDisposed.compare_exchange_strong(expected, false))
	{
		userData = userData_;
		receiveHandler = std::move(receiveHandler_);
		closeHandler = std::move(closeHandler_);
	}
}

void Connection::closeUser (mrudp_event_t event)
{
	auto expected = false;
	if (userDataDisposed.compare_exchange_strong(expected, true))
	{
		auto closeHandler_ = std::move(closeHandler);
		auto userData_ = std::move(userData);
			
		userData = nullptr;
		closeHandler = nullptr;
		receiveHandler = nullptr;

		if (closeHandler_)
		{
			xTraceChar(this, 0, '*');
			closeHandler_(userData_, event);
		}
	}
}

mrudp_error_code_t Connection::open ()
{
	imp = strong_thread(strong<imp::ConnectionImp>(strong_this(this)));

	socket->service->scheduler->allocate(
		finishTimeout,
		[this, self_=weak_this(this)]() {
			if (auto self = strong(self_))
			{
				finish();
			}
		}
	);

	mrudp_error_code_t status = MRUDP_OK;
	if (mrudp_failed(status = imp->open()))
	{
		return status;
	}
	
	probe.onStart(socket->service->clock.now());

#ifdef MRUDP_ENABLE_DEBUG_HOOK
	__debugHook();
#endif

	return MRUDP_OK;
}

#ifdef MRUDP_ENABLE_DEBUG_HOOK
void Connection::__debugHook()
{
	if (!sender.empty())
	{
		std::cerr << "__debugHook not empty!" << std::endl;
		xDebugLine();
	}
	
	if (statistics.statistics.reliable.frames.received == 0 && statistics.statistics.reliable.frames.sent == 0)
	{
		int x = 0;
		x++;
	}
	
	static int numToWaitFor = 512;
	if (statistics.statistics.reliable.frames.received < numToWaitFor && statistics.statistics.reliable.frames.received > 0)
	{
		int x = 0;
		x++;
	}
	
	if (!receiver.receiveQueue.queue.empty())
	{
		int y = 0;
		y++;
	}

	imp->setTimeout("debug", socket->service->clock.now() + toDuration(10),
		[this](){
			this->__debugHook();
		}
	);
}
#endif

void Connection::close()
{
	xLogDebug(logOfThis(this));

	sender.close();
	receiver.close();
	possiblyClose();
}

void Connection::possiblyClose ()
{
	if (closed)
		return;

	if (sender.status == Sender::CLOSED)
	if (receiver.status == Receiver::CLOSED)
	if (sender.empty())
	{
		close(closeReason);
	}
}

void Connection::fail (mrudp_event_t event)
{
	xTraceChar(this, 0, '%');

	closeReason = event;

	probe.close();
	receiver.fail();
	sender.fail();
	
	possiblyClose();
}

void Connection::close (mrudp_event_t event)
{
	xTraceChar(this, 0, '#');

	xLogDebug(logOfThis(this) << logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVar(userData));

	auto expected = false;
	if (closed.compare_exchange_strong(expected, true))
	{
		closeUser(event);
			
		finishWhenReady();
	}
}

void Connection::finishWhenReady()
{
	auto interval = toDuration(sender.rtt.duration * 3);
	finishTimeout.schedule(socket->service->clock.now() + interval);
}

void Connection::finish ()
{
	xTraceChar(this, 0, '@');
	socket->erase(this);
}

void Connection::receive(Packet &packet, const Address &remoteAddress)
{
	sLogDebug("mrudp::receive", logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVarV(packet.header.connection) << logVarV((char)packet.header.type) << logVarV(packet.header.id) << logVarV(packet.dataSize))

#ifdef MRUDP_ENABLE_CRYPTO
	if (crypto->onReceive(packet) == Discard)
	{
		sLogDebug("mrudp::receive", "decryption failed");
		xTraceChar(this, packet.header.id, '^');

		debug_assert(false);
		xDebugLine();
		return ;
	}
	
	sLogDebug("mrudp::receive", logVarV((char)packet.header.type) << logVarV(packet.header.id) << logVarV(packet.dataSize))
#endif

	if (handshake_options.onReceive(packet) == Discard)
		return ;

	xTraceChar(this, packet.header.id, (char)std::tolower((char)packet.header.type));
	auto now = socket->service->clock.now();

	statistics.onReceive(packet);

	if (networkPath.onReceive(packet, remoteAddress) == Discard)
		return ;

	if (handshake.onReceive(packet) == Discard)
		return ;
		
	probe.onReceive(now);
	sender.onReceive(packet);
	receiver.onReceive(packet);
}

void Connection::receive(char *buffer, int size, Reliability reliability)
{
	if (receiveHandler)
		receiveHandler(
			userData,
			buffer,
			size,
			reliability
		);

	statistics.onReceiveDataFrame(size, reliability);
}

bool Connection::canSend ()
{
#ifdef MRUDP_ENABLE_CRYPTO
	return crypto->canSend();
#else
	return remoteID > 0;
#endif
}

void Connection::send_(const PacketPtr &packet, Address *address)
{
	packet->header.connection = remoteID;
	
	if (packet->header.connection == 0)
	{
		auto packet_ = strong<Packet>();
		*packet_ = *packet;
		pushData(*packet_, id);
		
		sLogDebug("mrudp::send", logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVarV(packet_->header.connection) << logVarV(packet_->header.type) << logVarV(packet_->header.id) << logVarV(packet_->dataSize) << " with long ID");

		socket->send(packet_, this, address);
	}
	else
	{
		sLogDebug("mrudp::send", logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVarV(packet->header.connection) << logVarV((char)packet->header.type) << logVarV(packet->header.id) << logVarV(packet->dataSize));

		socket->send(packet, this, address);
	}

	probe.onSend(socket->service->clock.now());
}

void Connection::send(const PacketPtr &packet, Address *address)
{
	xTraceChar(this, packet->header.id, (char)packet->header.type);

	statistics.onSend(*packet);

	// the only time we should be using the longID, is when we are sending control packets
	debug_assert(
		(remoteID == 0 && (packet->header.type != DATA_RELIABLE && packet->header.type != DATA_UNRELIABLE)) ||
		(remoteID != 0)
	);

	if (handshake_options.onSend(*packet) == Discard)
		return ;

#ifdef MRUDP_ENABLE_CRYPTO
	if (crypto->onSend(*packet) == Discard)
	{
		sLogDebug("mrudp::send", "encryption failed");
		
		debug_assert(false);
		xDebugLine();
		return;
	}
#endif

	send_(packet, address);
}

void Connection::resend(const PacketPtr &packet, Address *address)
{
	// xTraceChar(this, packet->header.id, 'R', (char)packet->header.type);
	send_(packet, address);
	
	statistics.onResend(*packet);
}

ErrorCode Connection::send(const char *buffer, int size, Reliability reliability)
{
	xLogDebug(logOfThis(this));

	statistics.onSendDataFrame(size, reliability);

	return sender.send((const u8 *)buffer, size, reliability);
}

void Connection::onRemoteAddressChanged (const Address &remoteAddress_)
{
	remoteAddress = remoteAddress_;
	imp->onRemoteAddressChanged();
}

// -----------------------

ConnectionHandle newHandle(const StrongPtr<Connection> &connection)
{
	return (ConnectionHandle) new StrongPtr<Connection>(connection);
}

StrongPtr<Connection> toNative(ConnectionHandle handle_)
{
	if (!handle_)
		return nullptr;
		
	auto handle = (StrongPtr<Connection> *)handle_;
	return *handle;
}

StrongPtr<Connection> closeHandle (ConnectionHandle handle_)
{
	if (!handle_)
		return nullptr;
		
	StrongPtr<Connection> result;
	auto handle = (StrongPtr<Connection> *)handle_;
	std::swap(result, *handle);
	
	return result;
}

void deleteHandle (ConnectionHandle handle_)
{
	auto handle = (StrongPtr<Connection> *)handle_;
	delete handle;
}

} // namespace
} // namespace
