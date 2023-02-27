#include "Proxy.hpp"
#include "Base.h"
#include "Types.h"

#include <zlib.h>

namespace timprepscius::mrudp::proxy {

using ProxyID = uint32_t;
using ProxyPacketSize = uint16_t;
using PayloadSize = uint32_t;
using PayloadPacketSize = uint32_t;

constexpr ProxyID ProxyID_SERVER = 0xFF000000;
constexpr ProxyID ProxyID_SERVER_LBITS = 24;
constexpr ProxyID ProxyID_CONNECTION = ~(ProxyID_SERVER);

const int proxyProbeDelay = 30;

struct ProxyMode{
	enum Enum : u8 {
		NONE,
		WIRE,
		CONNECTION
	};
} ;

struct ProxyPacketMode {
	enum Enum : u8 {
		OPEN,
		CLOSE,
		RELIABLE,
		UNRELIABLE
	} ;
} ;


PACK (
	struct ProxyPacketHeader {
		ProxyPacketSize size;
		ProxyID id;
		ProxyPacketMode::Enum type;
	}
);

struct Proxy;

struct ProxyConnection {
	Proxy *proxy;
	ProxyID id;
	ProxyMode::Enum mode;
	mrudp_connection_t connection = nullptr;
} ;

struct Proxy {
	bool closed;
	StrongPtr<Proxy> retain;
	
	mrudp_socket_t clientSocket;
	mrudp_socket_t serverSocket;
	mrudp_proxy_options_t options;
	std::thread ticker;
	
	RecursiveMutex mutex;
	mrudp_connection_t wire;
	
	SizedVector<char> out;
	CircularBuffer<char> in;
	
	SizedVector<char> scratch[2];
	
	ProxyID nextProxyID;
	
	OrderedMap<ProxyID, ProxyConnection> connections;
} ;

void incrementProxyID(ProxyID &id)
{
	auto server = id & ProxyID_SERVER;
	
	id++;
	id = server | (id & ProxyID_CONNECTION);
}

struct DataIn {
	char *data;
	int remaining;
} ;

struct DataOut {
	char *data;
	int remaining;
	int size = 0;
} ;

template<typename V>
mrudp_error_code_t write(V &queue, const char *data, size_t size)
{
	auto at = queue.size();
	queue.resize(queue.size() + size);
	core::mem_copy(queue.data() + at, data, size);
	
	return MRUDP_OK;
}

template<>
mrudp_error_code_t write(DataOut &out, const char *data, size_t size)
{
	if (size > out.remaining)
		return -1;
		
	out.remaining -= size;
	mem_copy(out.data + out.size, data, size);
	out.size += size;
	
	return MRUDP_OK;
}

template<typename V, typename T>
void write(V &queue, const T &t)
{
	write(queue, (const char *)&t, sizeof(T));
}

mrudp_error_code_t read(DataIn &in, char *data, int size)
{
	if (size > in.remaining)
		return -1;
		
	in.remaining -= size;
	mem_copy(data, in.data, size);
	in.data += size;
	
	return MRUDP_OK;
}

mrudp_error_code_t skip(DataIn &in, int size)
{
	if (size > in.remaining)
		return -1;

	in.remaining -= size;
	in.data += size;
	
	return MRUDP_OK;
}

template<typename V, typename T>
mrudp_error_code_t read(V &v, T &t)
{
	return read(v, (char *)&t, sizeof(T));
}


template<typename V>
void write(V &v, const mrudp_addr_t &a)
{
	// TODO: this need to be rewritten with endianness and stuff
	char type = '4';
	write(v, type);

	static_assert(sizeof(a.v4.sin_addr) == 4);
	write(v, a.v4.sin_addr);

	static_assert(sizeof(a.v4.sin_port) == 2);
	write(v, a.v4.sin_port);
}

template<typename V>
mrudp_error_code_t read(V &v, mrudp_addr_t &a)
{
	// TODO: this need to be rewritten with endianness and stuff
	char type = '4';
	
	if (mrudp_failed(read(v, type)))
		return -1;
		
	if (type != '4')
		return -1;
	
	#ifdef SYS_APPLE
		a.v4.sin_len = sizeof(a.v4);
	#endif
	
	a.v4.sin_family = AF_INET;
	static_assert(sizeof(a.v4.sin_addr) == 4);
	if (mrudp_failed(read(v, a.v4.sin_addr)))
		return -1;
	
	static_assert(sizeof(a.v4.sin_port) == 2);
	if (mrudp_failed(read(v, a.v4.sin_port)))
		return -1;
	
	return 0;
}


mrudp_error_code_t proxy_send_queue(Proxy *proxy, ProxyConnection *connection, ProxyPacketMode::Enum mode, char *data, size_t size)
{
	core_only(debug_assert(proxy->mutex.locked_by_caller()));
	
	ProxyPacketHeader header;
	header.size = size;
	header.type = mode;
	header.id = connection->id;
	
	write(proxy->out, header);
	write(proxy->out, data, size);
	
	return MRUDP_OK;
}

mrudp_error_code_t proxy_receive_queue(Proxy *proxy, char *data, size_t size)
{
	core_only(debug_assert(proxy->mutex.locked_by_caller()));
	
	sLogDebug("mrudp::proxy", logVar(size));
	
	proxy->in.write(data, size);
	
	return MRUDP_OK;
}

mrudp_error_code_t on_connection_packet(void *connection_, char *data, int size, int is_reliable);
mrudp_error_code_t on_connection_close(void *connection_, mrudp_event_t event);

ProxyConnection &on_connection_open(Proxy *proxy, ProxyMode::Enum mode, ProxyID id, mrudp_addr_t *address)
{
	auto &proxyConnection = proxy->connections[id];
	proxyConnection.id = id;
	proxyConnection.proxy = proxy;
	proxyConnection.mode = mode;

	mrudp_connection_options_t options = mrudp_default_connection_options();
	options.coalesce_reliable.mode = MRUDP_COALESCE_PACKET;
	options.coalesce_reliable.delay_ms = 5;
	options.coalesce_unreliable.mode = MRUDP_COALESCE_NONE;
	options.probe_delay_ms = proxyProbeDelay;

	auto connection = mrudp_connect_ex(proxy->clientSocket, address, &options, &proxyConnection, on_connection_packet, on_connection_close);
	proxyConnection.connection = connection;
	
	return proxyConnection;
}

void initiate_connection_close(Proxy *proxy, ProxyID id)
{
	core_only(debug_assert(proxy->mutex.locked_by_caller()));
//	auto lock = lock_of(proxy->mutex);
	
	auto i = proxy->connections.find(id);
	if (i == proxy->connections.end())
		return;
		
	auto &connection = i->second;
	mrudp_connection_t retain = nullptr;
	std::swap(retain, connection.connection);
	
	mrudp_close_connection(retain);
}

void on_connection_close(Proxy *proxy, ProxyConnection *connection)
{
	auto i = proxy->connections.find(connection->id);
	if (i == proxy->connections.end())
		return;
		
	proxy->connections.erase(i);
}

mrudp_error_code_t on_wire_packet(Proxy *proxy, char *data_, int size_)
{
	DataIn in { data_, size_ };
	
	while (in.remaining)
	{
		ProxyPacketHeader header;
		if (mrudp_failed(read(in, header)))
		{
			debug_assert(false);
			return -1;
		}
		
		debug_assert(in.remaining >= header.size);
		
		switch (header.type)
		{
		case ProxyPacketMode::OPEN:
			mrudp_addr_t address;
			if (mrudp_failed(read(in, address)))
			{
				debug_assert(false);
				return -1;
			}
				
			sLogDebug("mrudp::proxy", "OPEN " << logVar(toString(address)));
			on_connection_open(proxy, ProxyMode::CONNECTION, header.id, &address);
		break;
		
		case ProxyPacketMode::CLOSE:
			initiate_connection_close(proxy, header.id);
			debug_assert(header.size == 0);
		break;
		
		case ProxyPacketMode::RELIABLE:
		case ProxyPacketMode::UNRELIABLE:
		{
			auto connection_ = proxy->connections.find(header.id);
			if (connection_ != proxy->connections.end())
			{
				sLogDebug("mrudp::proxy::detail", logVar(connection_->second.id) << logVarV(header.size));
				auto connection = connection_->second.connection;
				mrudp_send(connection, in.data, header.size, header.type == ProxyPacketMode::RELIABLE);
			}
			else
			{
				// debug_assert(header.type == ProxyPacketMode::UNRELIABLE);
			}
			
			skip(in, header.size);
		}
		break;
		
		}
	}

	return 0;
}

mrudp_error_code_t on_wire_close(Proxy *proxy, ProxyConnection *connection, mrudp_event_t)
{
	on_connection_close(proxy, connection);

	return 0;
}

mrudp_error_code_t on_proxy_packet(Proxy *proxy, ProxyConnection *connection, char *data, int size, int is_reliable)
{
	return proxy_send_queue(
		proxy, connection,
		is_reliable ? ProxyPacketMode::RELIABLE : ProxyPacketMode::UNRELIABLE,
		data, size
	);
}

mrudp_error_code_t on_proxy_close(Proxy *proxy, ProxyConnection *connection, mrudp_event_t)
{
	proxy_send_queue(
		proxy, connection,
		ProxyPacketMode::CLOSE,
		nullptr, 0
	);

	on_connection_close(proxy, connection);
	
	return 0;
}

mrudp_error_code_t on_mode_packet(Proxy *proxy, ProxyConnection *connection, char *data_, int size_, int is_reliable)
{
	core_only(debug_assert(proxy->mutex.locked_by_caller()));

	DataIn in { data_, size_ };

	mrudp_proxy_magic_t magic;
	if (mrudp_failed(read(in, magic)))
		return -1;
		
	if (magic == proxy->options.magic_wire)
	{
		connection->mode = ProxyMode::WIRE;
		proxy->wire = connection->connection;
		
		mrudp_connection_options_t options;
		mrudp_connection_options(connection->connection, &options);

		options.coalesce_reliable.mode = MRUDP_COALESCE_STREAM;
		mrudp_connection_options_set(connection->connection, &options);
		
		sLogRelease("mrudp::proxy::run", "setting WIRE " << logVar(connection->id));
		
		// process any remaining
		return on_connection_packet(connection, in.data, in.remaining, is_reliable);
	}
	else
	if (magic == proxy->options.magic_connection)
	{
		mrudp_addr_t addr;
		if (mrudp_failed(read(in, addr)))
		{
			debug_assert(false);
			return -1;
		}

		char buffer[256];
		DataOut out { buffer, 256 };
		write(out, addr);
		
		if (mrudp_failed(proxy_send_queue(proxy, connection, ProxyPacketMode::OPEN, out.data, out.size)))
		{
			debug_assert(false);
			return -1;
		}
		
		connection->mode = ProxyMode::CONNECTION;

		sLogDebug("mrudp::proxy", "setting CONNECTION " << logVar(connection->id));

		// process any remaining
		return on_connection_packet(connection, in.data, in.remaining, is_reliable);
	}
	
	return -1;
}


mrudp_error_code_t on_connection_packet(void *connection_, char *data, int size, int is_reliable)
{
	if (size == 0)
		return 0;
		
	auto connection = (ProxyConnection *)connection_;
	auto proxy = (Proxy *)connection->proxy;
	
	auto lock = lock_of(proxy->mutex);
	
	switch (connection->mode) {
	case ProxyMode::NONE:
		return on_mode_packet(proxy, connection, data, size, is_reliable);
	case ProxyMode::WIRE:
		return proxy_receive_queue(proxy, data, size);
	case ProxyMode::CONNECTION:
		return on_proxy_packet(proxy, connection, data, size, is_reliable);
	default: ;
	}

	return -1;
}

mrudp_error_code_t on_connection_close(void *connection_, mrudp_event_t event)
{
	auto connection = (ProxyConnection *)connection_;
	auto proxy = (Proxy *)connection->proxy;

	mrudp_close_connection(connection->connection);
	connection->connection = nullptr;
	
	auto lock = lock_of(proxy->mutex);
	
	switch (connection->mode) {
	case ProxyMode::WIRE:
		return on_wire_close(proxy, connection, event);
	case ProxyMode::CONNECTION:
		return on_proxy_close(proxy, connection, event);
	default: ;
		return -1;
	}
	return -1;
}

mrudp_error_code_t on_socket_close(void *proxy_, mrudp_event_t event)
{
	return 0;
}

mrudp_error_code_t on_socket_accept(void *proxy_, mrudp_connection_t connection)
{
	auto proxy = (Proxy *)proxy_;
	auto proxyID = proxy->nextProxyID;
	incrementProxyID(proxy->nextProxyID);
	
	auto &proxyConnection = proxy->connections[proxyID];
	proxyConnection.id = proxyID;
	proxyConnection.proxy = proxy;
	proxyConnection.mode = ProxyMode::NONE;
	proxyConnection.connection = connection;
	
	mrudp_connection_options_t options = mrudp_default_connection_options();
	options.coalesce_reliable.mode = MRUDP_COALESCE_PACKET;
	options.coalesce_unreliable.mode = MRUDP_COALESCE_NONE;
	options.probe_delay_ms = proxyProbeDelay;
	
	return mrudp_accept_ex(
		connection,
		&options,
		&proxyConnection,
		on_connection_packet,
		on_connection_close
	);
}

void proxy_tick(Proxy *proxy)
{
	if (!proxy->wire)
		return;

	using WasCompressed = u8;
	using UncompressedSize = PayloadSize;
	// TODO: go through and fast fail
	auto lock = lock_of(proxy->mutex);
	if (!proxy->out.empty())
	{
		proxy->scratch[0].resize(sizeof(PayloadPacketSize) + sizeof(WasCompressed) + sizeof(UncompressedSize) + proxy->out.size());

		auto *size = (PayloadPacketSize *)proxy->scratch[0].data();
		auto *wasCompressed = (WasCompressed *)(size+1);
		auto *uncompressedSize = (u8 *)(wasCompressed + 1);
		auto *compressionDest = (u8 *)(uncompressedSize + sizeof(UncompressedSize));
		
		const Bytef *source = (const Bytef *)proxy->out.data();
		uLongf sourceLen = proxy->out.size();
		Bytef *dest = (Bytef *)compressionDest;
		uLongf destLen = proxy->out.size();
		int level = proxy->options.compression_level;
		
		PayloadSize payloadSize = 0;
		payloadSize += sizeof(WasCompressed);

		if (level > 0 && compress2(dest, &destLen, source, sourceLen, level) == Z_OK)
		{
			UncompressedSize uncompressedSize_ = (PayloadSize)sourceLen;

			*wasCompressed = 1;

			core::small_copy((char *)uncompressedSize, (const char *)&uncompressedSize_, sizeof(UncompressedSize));
			payloadSize += sizeof(UncompressedSize);
			payloadSize += destLen;

			sLogRelease("mrudp::proxy::compress", logVar(sourceLen) << logVar(destLen));
		}
		else
		{
			sLogRelease("mrudp::proxy::compress", logVar(sourceLen) << "no compression");

			*wasCompressed = 0;
			
			auto *uncompressedDest = ((u8 *)wasCompressed) + sizeof(WasCompressed);
			core::mem_copy((char *)uncompressedDest, proxy->out.data(), sourceLen);
			payloadSize += sourceLen;
		}

		*size = payloadSize;
		mrudp_send(proxy->wire, proxy->scratch[0].data(), sizeof(PayloadPacketSize) + payloadSize, 1);
		proxy->out.resize(0);
	}
	
	sLogDebug("mrudp::proxy::detail", logVar(proxy->in.size()));
	while (proxy->in.size() > sizeof(PayloadPacketSize))
	{
		PayloadPacketSize packetSize;
		proxy->in.peek((char *)&packetSize, sizeof(PayloadPacketSize));
		sLogDebug("mrudp::proxy", logVar(packetSize));
		
		debug_assert(packetSize > 0);
		
		if (proxy->in.size() < packetSize + sizeof(PayloadPacketSize))
			break;

		proxy->in.skip(sizeof(PayloadPacketSize));

		auto payloadSize = packetSize;
		
		WasCompressed wasCompressed;
		proxy->in.read((char *)&wasCompressed, 1);
		payloadSize -= sizeof(WasCompressed);

		if (wasCompressed == 0)
		{
			proxy->scratch[0].resize(payloadSize);
			proxy->in.read(proxy->scratch[0].data(), payloadSize);

			on_wire_packet(proxy, proxy->scratch[0].data(), payloadSize);
		}
		else
		{
			PayloadSize uncompressedSize = 0;
			proxy->in.read((char *)&uncompressedSize, sizeof(PayloadSize));
			payloadSize -= sizeof(PayloadSize);

			proxy->scratch[0].resize(payloadSize);
			
			sLogRelease("mrudp::proxy::compress", logVar(payloadSize) << logVar(uncompressedSize));

			proxy->in.read(proxy->scratch[0].data(), payloadSize);
			proxy->scratch[1].resize(uncompressedSize);
			
			const Bytef *source = (const Bytef *)proxy->scratch[0].data();
			uLongf sourceLen = payloadSize;
			Bytef *dest = (Bytef *)proxy->scratch[1].data();
			uLongf destLen = uncompressedSize;
			
			uncompress(dest, &destLen, source, sourceLen);
			debug_assert(destLen == uncompressedSize);
			
			on_wire_packet(proxy, proxy->scratch[1].data(), uncompressedSize);
		}
	}
}

void ticker (Proxy *proxy)
{
	while (!proxy->closed)
	{
		proxy_tick(proxy);
		std::this_thread::sleep_for(std::chrono::milliseconds(proxy->options.tick_interval_ms));
	}
}

void *open(
	mrudp_service_t service,
	const mrudp_addr_t *from, const mrudp_addr_t *on, const mrudp_addr_t *to,
	const mrudp_proxy_options_t *options,
	mrudp_addr_t *from_bound, mrudp_addr_t *on_bound
)
{
	auto proxy_retain = strong<Proxy>();
	auto *proxy = ptr_of(proxy_retain);
	proxy->retain = proxy_retain;
	
	proxy->closed = false;
	proxy->options = *options;
	proxy->clientSocket = mrudp_socket(service, from);
	proxy->serverSocket = on ? mrudp_socket(service, on) : proxy->clientSocket;
	proxy->wire = nullptr;
	
	if (from_bound)
		mrudp_socket_addr(proxy->clientSocket, from_bound);

	if (on_bound)
		mrudp_socket_addr(proxy->serverSocket, on_bound);

	if (to)
	{
		proxy->nextProxyID = 0;

		mrudp_connection_options_t options = mrudp_default_connection_options();
		options.coalesce_reliable.mode = MRUDP_COALESCE_STREAM;
		options.coalesce_reliable.delay_ms = 0;
		options.coalesce_unreliable.mode = MRUDP_COALESCE_NONE;
		options.probe_delay_ms = proxyProbeDelay;

		auto id = proxy->nextProxyID;
		incrementProxyID(proxy->nextProxyID);
		
		auto &proxyConnection = proxy->connections[id];
		proxyConnection.id = id;
		proxyConnection.proxy = proxy;
		proxyConnection.mode = ProxyMode::WIRE;

		proxy->wire = mrudp_connect_ex(
			proxy->serverSocket,
			to, &options,
			&proxyConnection,
			on_connection_packet,
			on_connection_close
		);
		
		auto wireMagic = proxy->options.magic_wire;
		mrudp_send(proxy->wire, (char *)&wireMagic, sizeof(wireMagic), 1);
	}
	else
	{
		// TODO: this needs to change
		// but to what?  shortConnectionID? it needs to be informed by the remote I guess?
		// could be random number?
		proxy->nextProxyID = ProxyID_SERVER;
	}

	mrudp_listen(proxy->clientSocket, proxy, nullptr, on_socket_accept, on_socket_close);
	
	if (proxy->serverSocket != proxy->clientSocket)
		mrudp_listen(proxy->serverSocket, proxy, nullptr, on_socket_accept, on_socket_close);

	proxy->ticker = std::thread([proxy_weak=weak(proxy_retain)]() {
		if (auto proxy = strong(proxy_weak))
			ticker(ptr_of(proxy));
	});

	return proxy;
}

void close(void *proxy_)
{
	Proxy *proxy = (Proxy *)proxy_;
	
	proxy->closed = true;
	proxy->ticker.join();
		
	mrudp_close_connection(proxy->wire);
	proxy->wire = nullptr;

	if (proxy->serverSocket != proxy->clientSocket)
		mrudp_close_socket(proxy->serverSocket);

	decltype(proxy->connections) connections;
	mrudp_close_socket(proxy->clientSocket);
	{
		auto lock = lock_of(proxy->mutex);
		connections = std::move(proxy->connections);
	}

	for (auto &[id, connection] : connections)
		mrudp_close_connection(connection.connection);

	proxy->retain = nullptr;
}

mrudp_error_code_t connect (mrudp_connection_t connection, const mrudp_addr_t *remote, mrudp_proxy_magic_t connectionMagic)
{
	// send the first packet
	std::vector<char> data;
	data.reserve(128);
	
	write(data, (char *)&connectionMagic, sizeof(connectionMagic));
	write(data, *remote);
	
	return mrudp_send(connection, data.data(), (int)data.size(), 1);
}

mrudp_proxy_options_t options_default()
{
	return {
		.magic_wire = 42,
		.magic_connection = 13,
		.tick_interval_ms = 250,
		.compression_level = 9
	} ;
}

}

