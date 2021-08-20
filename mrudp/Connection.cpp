#include "Connection.h"
#include "Implementation.h"

#include <iostream>

namespace timprepscius {
namespace mrudp {

static std::atomic<size_t> NumConnection = 0;

Connection::Connection(const StrongPtr<Socket> &socket_, LongConnectionID id_, const Address &remoteAddress_, ShortConnectionID localID_) :
	socket(socket_),
	id(id_),
	localID(localID_),
	remoteAddress(remoteAddress_),
	sender(this),
	receiver(this),
	probe(this),
	handshake(this),
	options(socket->service->imp->options.connection)
{
	#ifdef MRUDP_ENABLE_CRYPTO
		crypto = strong<ConnectionCrypto>(socket->service->crypto);
	#endif

	sLogDebug("mrudp::life_cycle", logOfThis(this) << ++NumConnection);
	xLogDebug(logOfThis(this) << logLabel("socket connection") << logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)));
}

Connection::~Connection ()
{
	xTraceChar(this, 0, '~');
	

	xLogDebug(logOfThis(this));
	sLogDebug("mrudp::life_cycle", logOfThis(this) << --NumConnection);
	
	imp = nullptr;
}

void Connection::openUser(const ConnectionOptions *options_, void *userData_, mrudp_receive_callback_fn receiveHandler_, mrudp_close_callback_fn closeHandler_)
{
	if (options_)
		options = merge(*options_, options);

	auto lock = lock_of(userDataMutex);
	
	userData = userData_;
	receiveHandler = receiveHandler_;
	closeHandler = closeHandler_;
}

void Connection::closeUser ()
{
	auto lock = lock_of(userDataMutex);
	
	userData = nullptr;
	receiveHandler = nullptr;
	closeHandler = nullptr;
}

void Connection::open ()
{
	imp = strong_thread(strong<imp::ConnectionImp>(strong_this(this)));
	imp->open();
	
	probe.onStart(socket->service->clock.now());

#ifdef MRUDP_ENABLE_DEBUG_HOOK
	__debugHook();
#endif
}

#ifdef MRUDP_ENABLE_DEBUG_HOOK
void Connection::__debugHook()
{
	if (!sender.empty())
	{
		std::cerr << "__debugHook not empty!" << std::endl;
		xDebugLine();
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

	xLogDebug(logOfThis(this) << logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) logVar(closeHandler) << logVar(userData));

	auto expected = false;
	if (closed.compare_exchange_strong(expected, true))
	{
		mrudp_close_callback_fn closeHandler_ = nullptr;
		void *userData_ = nullptr;

		{
			auto lock = lock_of(userDataMutex);
			closeHandler_ = std::move(closeHandler);
			userData_ = std::move(userData);
				
			userData = nullptr;
			closeHandler = nullptr;
			receiveHandler = nullptr;
		}

		if (closeHandler_)
		{
			xTraceChar(this, 0, '*');
			closeHandler_(userData_, event);
		}
			
		finishWhenReady();
	}
}

void Connection::finishWhenReady()
{
	auto interval = toDuration(sender.rtt.duration * 3);
	imp->setTimeout(
		"finish",
		socket->service->clock.now() + interval,
		[this, self_=weak_this(this)]() {
			if (auto self = strong(self_))
			{
				finish();
			}
		}
	);
}

void Connection::finish ()
{
	xTraceChar(this, 0, '@');
	socket->erase(this);
}

void Connection::receive(Packet &packet)
{
	sLogDebug("mrudp::receive", logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVar(packet.header.connection) << logVar((char)packet.header.type) << logVar(packet.header.id) << logVar(packet.dataSize))

#ifdef MRUDP_ENABLE_CRYPTO
	if (!crypto->decrypt(packet))
	{
		sLogDebug("mrudp::receive", "decryption failed");
		xTraceChar(this, packet.header.id, '^');

		debug_assert(false);
		xDebugLine();
		return ;
	}
	
	sLogDebug("mrudp::receive", logVar((char)packet.header.type) << logVar(packet.header.id) << logVar(packet.dataSize))
#endif

	// this should be moved somewhere else
	statistics.onReceive(packet);

	xTraceChar(this, packet.header.id, (char)std::tolower((char)packet.header.type));
	handshake.onPacket(packet);

	auto now = socket->service->clock.now();
	probe.onReceive(now);
	
	sender.onPacket(packet);
	receiver.onPacket(packet);
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
	return crypto->canEncrypt();
#else
	return true;
#endif
}

void Connection::send_(const PacketPtr &packet)
{
	packet->header.connection = remoteID;
	
	if (packet->header.connection == 0)
	{
		auto packet_ = strong<Packet>();
		*packet_ = *packet;
		pushData(*packet_, id);
		
		sLogDebug("mrudp::send", logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVar(packet_->header.connection) << logVar(packet_->header.type) << logVar(packet_->header.id) << logVar(packet_->dataSize) << " with long ID");

		socket->send(packet_, remoteAddress, this);
	}
	else
	{
		sLogDebug("mrudp::send", logLabelVar("local", toString(socket->getLocalAddress())) << logLabelVar("remote", toString(remoteAddress)) << logVar(packet->header.connection) << logVar((char)packet->header.type) << logVar(packet->header.id) << logVar(packet->dataSize));

		socket->send(packet, remoteAddress, this);
	}

	probe.onSend(socket->service->clock.now());
}

void Connection::send(const PacketPtr &packet)
{
	xTraceChar(this, packet->header.id, (char)packet->header.type);

	statistics.onSend(*packet);

#ifdef MRUDP_ENABLE_CRYPTO
	if (!crypto->encrypt(*packet))
	{
		sLogDebug("mrudp::send", "encryption failed");
		
		debug_assert(false);
		xDebugLine();
		return;
	}
#endif

	send_(packet);
}

void Connection::resend(const PacketPtr &packet)
{
	// xTraceChar(this, packet->header.id, 'R', (char)packet->header.type);
	send_(packet);
	
	statistics.onResend(*packet);
}

void Connection::send(const char *buffer, int size, Reliability reliability)
{
	xLogDebug(logOfThis(this));

	statistics.onSendDataFrame(size, reliability);

	sender.send((const u8 *)buffer, size, reliability);
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
