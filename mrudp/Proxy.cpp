#include "Proxy.hpp"
#include "base/Core.h"
#include "base/Pack.h"
#include "base/Thread.h"
#include "base/Misc.h"

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
	mrudp_connection_t connection;
} ;

struct Proxy {
	bool closed;
	StrongPtr<Proxy> retain;
	
	mrudp_socket_t socket;
	mrudp_proxy_magic_t wireMagic, connectionMagic;
	std::thread ticker;
	
	Mutex mutex;
	mrudp_connection_t wire;
	
	SizedVector<char> out;
	CircularBuffer<char> in;
	
	SizedVector<char> scratch[2];
	
	ProxyID nextProxyID;
	
	core::Map<ProxyID, ProxyConnection> connections;
} ;

void incrementProxyID(ProxyID &id)
{
	auto server = id & ProxyID_SERVER;
	
	id++;
	id = server | (id & ProxyID_CONNECTION);
}

template<typename V>
void write(V &queue, char *data, size_t size)
{
	auto at = queue.size();
	queue.resize(queue.size() + size);
	core::mem_copy(queue.data() + at, data, size);
}

mrudp_error_code_t proxy_send_queue(Proxy *proxy, ProxyConnection *connection, ProxyPacketMode::Enum mode, char *data, size_t size)
{
	auto lock = lock_of(proxy->mutex);
	ProxyPacketHeader header;
	header.size = size;
	header.type = mode;
	header.id = connection->id;
	
	write(proxy->out, (char *)&header, sizeof(header));
	write(proxy->out, data, size);
	
	return MRUDP_OK;
}

mrudp_error_code_t proxy_receive_queue(Proxy *proxy, char *data, size_t size)
{
	sLogDebug("mrudp::proxy", logVar(size));
	
	proxy->in.write(data, size);
	
	return MRUDP_OK;
}

mrudp_error_code_t read(char *into, char *&data, int &remaining, int size)
{
	if (size > remaining)
		return -1;
		
	remaining -= size;
	memcpy(into, data, size);
	data += size;
	
	return MRUDP_OK;
}

template<typename T>
mrudp_error_code_t read(T &into, char *&data, int &size)
{
	return read((char *)&into, data, size, sizeof(T));
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

	auto connection = mrudp_connect_ex(proxy->socket, address, &options, &proxyConnection, on_connection_packet, on_connection_close);
	proxyConnection.connection = connection;
	
	return proxyConnection;
}

void on_connection_close(Proxy *proxy, ProxyID id)
{
	auto lock = lock_of(proxy->mutex);
	
	auto i = proxy->connections.find(id);
	if (i == proxy->connections.end())
		return;
		
	auto connection = i->second.connection;
	mrudp_close_connection(connection);
	
	proxy->connections.erase(i);
}

mrudp_error_code_t on_wire_packet(Proxy *proxy, char *data, int size)
{
	while (size)
	{
		ProxyPacketHeader header;
		if (mrudp_failed(read(header, data, size)))
		{
			debug_assert(false);
			return -1;
		}
		
		debug_assert(size >= header.size);
		
		switch (header.type)
		{
		case ProxyPacketMode::OPEN:
			mrudp_addr_t address;
			if (mrudp_failed(read(address, data, size)))
			{
				debug_assert(false);
				return -1;
			}
				
			on_connection_open(proxy, ProxyMode::CONNECTION, header.id, &address);
		break;
		
		case ProxyPacketMode::CLOSE:
			on_connection_close(proxy, header.id);
		break;
		
		case ProxyPacketMode::RELIABLE:
		case ProxyPacketMode::UNRELIABLE:
		{
			auto connection_ = proxy->connections.find(header.id);
			if (connection_ == proxy->connections.end())
			{
				debug_assert(false);
				return -1;
			}
				
			sLogDebug("mrudp::proxy::detail", logVar(connection_->second.id) << logVar(size));
			auto connection = connection_->second.connection;
			mrudp_send(connection, data, header.size, header.type == ProxyPacketMode::RELIABLE);
			data += header.size;
			size -= header.size;
		}
		break;
		
		}
	}

	return 0;
}

mrudp_error_code_t on_wire_close(Proxy *proxy, ProxyConnection *connection, mrudp_event_t)
{
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
	return 0;
}

mrudp_error_code_t on_mode_packet(Proxy *proxy, ProxyConnection *connection, char *data, int size, int is_reliable)
{
	mrudp_proxy_magic_t magic;
	if (mrudp_failed(read(magic, data, size)))
		return -1;
		
	if (magic == proxy->wireMagic)
	{
		connection->mode = ProxyMode::WIRE;
		proxy->wire = connection->connection;
		
		mrudp_connection_options_t options;
		mrudp_connection_options(connection->connection, &options);

		options.coalesce_reliable.mode = MRUDP_COALESCE_STREAM;
		mrudp_connection_options_set(connection->connection, &options);
		
		sLogDebug("mrudp::proxy", "setting WIRE " << logVar(connection->id));
		
		// process any remaining
		return on_connection_packet(connection, data, size, is_reliable);
	}
	else
	if (magic == proxy->connectionMagic)
	{
		mrudp_addr_t addr;
		if (mrudp_failed(read(addr, data, size)))
		{
			debug_assert(false);
			return -1;
		}

		if (mrudp_failed(proxy_send_queue(proxy, connection, ProxyPacketMode::OPEN, (char *)&addr, sizeof(addr))))
		{
			debug_assert(false);
			return -1;
		}
		
		connection->mode = ProxyMode::CONNECTION;

		sLogDebug("mrudp::proxy", "setting CONNECTION " << logVar(connection->id));

		// process any remaining
		return on_connection_packet(connection, data, size, is_reliable);
	}
	
	return -1;
}


mrudp_error_code_t on_connection_packet(void *connection_, char *data, int size, int is_reliable)
{
	if (size == 0)
		return 0;
		
	auto connection = (ProxyConnection *)connection_;
	auto proxy = (Proxy *)connection->proxy;
	
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

mrudp_error_code_t on_connection_accept(void *proxy_, mrudp_connection_t connection)
{
	auto proxy = (Proxy *)proxy_;
	auto proxyID = proxy->nextProxyID;
	incrementProxyID(proxy->nextProxyID);
	
	auto &proxyConnection = proxy->connections[proxyID];
	proxyConnection.id = proxyID;
	proxyConnection.proxy = proxy;
	proxyConnection.mode = ProxyMode::NONE;
	proxyConnection.connection = connection;
	
	mrudp_connection_options_t options;
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
	using WasCompressed = u8;
	using UncompressedSize = PayloadSize;
	// TODO: go through and fast fail
	auto lock = lock_of(proxy->mutex);
	if (!proxy->out.empty())
	{
		proxy->scratch[0].resize(proxy->out.size() + sizeof(PayloadPacketSize) + sizeof(WasCompressed) + sizeof(UncompressedSize));

		auto *size = (PayloadPacketSize *)proxy->scratch[0].data();
		auto *wasCompressed = (WasCompressed *)(size+1);
		auto *uncompressedSize = (u8 *)(wasCompressed + 1);
		auto *compressionDest = (u8 *)(uncompressedSize + sizeof(UncompressedSize));
		
		const Bytef *source = (const Bytef *)proxy->out.data();
		uLongf sourceLen = proxy->out.size();
		Bytef *dest = (Bytef *)compressionDest;
		uLongf destLen = proxy->out.size();
		int level = 1;
		
		PayloadSize payloadSize = 0;
		payloadSize += sizeof(WasCompressed);

		if (compress2(dest, &destLen, source, sourceLen, level) == Z_OK)
		{
			UncompressedSize uncompressedSize_ = (PayloadSize)sourceLen;

			*wasCompressed = 1;

			core::small_copy((char *)uncompressedSize, (const char *)&uncompressedSize_, sizeof(UncompressedSize));
			payloadSize += sizeof(UncompressedSize);
			payloadSize += destLen;
			
			*size = (PayloadSize)(destLen + sizeof(WasCompressed) + sizeof(UncompressedSize));
			
			sLogDebug("mrudp::proxy::compress", logVar(sourceLen) << logVar(destLen));
		}
		else
		{
			sLogDebug("mrudp::proxy::compress", logVar(sourceLen) << "no compression");

			*wasCompressed = 0;
			
			auto *uncompressedDest = (u8 *)(uncompressedSize + 1);
			core::mem_copy((char *)uncompressedDest, proxy->out.data(), sourceLen);
			payloadSize += sourceLen;
		}

		*size = payloadSize;
		mrudp_send(proxy->wire, proxy->scratch[0].data(), sizeof(PayloadPacketSize) + *size, 1);
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
			
			sLogDebug("mrudp::proxy::compress", logVar(payloadSize) << logVar(uncompressedSize));

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
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		proxy_tick(proxy);
	}
}

void *open(
	mrudp_service_t service,
	const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound,
	mrudp_proxy_magic_t wireMagic, mrudp_proxy_magic_t connectionMagic)
{
	auto proxy_retain = strong<Proxy>();
	auto *proxy = ptr_of(proxy_retain);
	proxy->retain = proxy_retain;
	
	proxy->closed = false;
	proxy->wireMagic = wireMagic;
	proxy->connectionMagic = connectionMagic;
	proxy->socket = mrudp_socket(service, from);
	proxy->wire = nullptr;
	
	proxy->ticker = std::thread([proxy_retain=weak(proxy_retain)]() {
		if (auto proxy = strong(proxy_retain))
			ticker(ptr_of(proxy));
	});
	
	if (bound)
		mrudp_socket_addr(proxy->socket, bound);
	
	if (to)
	{
		proxy->nextProxyID = 0;

		mrudp_connection_options_t options;
		options.coalesce_reliable.mode = MRUDP_COALESCE_STREAM;
		options.coalesce_unreliable.mode = MRUDP_COALESCE_NONE;
		options.probe_delay_ms = proxyProbeDelay;

		auto id = proxy->nextProxyID;
		incrementProxyID(proxy->nextProxyID);
		
		auto &proxyConnection = proxy->connections[id];
		proxyConnection.id = id;
		proxyConnection.proxy = proxy;
		proxyConnection.mode = ProxyMode::WIRE;

		proxy->wire = mrudp_connect_ex(
			proxy->socket, to, &options,
			&proxyConnection,
			on_connection_packet,
			on_connection_close
		);
		
		mrudp_send(proxy->wire, (char *)&wireMagic, sizeof(wireMagic), 1);
	}
	else
	{
		// TODO: this needs to change
		// but to what?  shortConnectionID? it needs to be informed by the remote I guess?
		// could be random number?
		proxy->nextProxyID = ProxyID_SERVER;
	}

	mrudp_listen(proxy->socket, proxy, nullptr, on_connection_accept, on_connection_close);

	return proxy;
}

void close(void *proxy_)
{
	Proxy *proxy = (Proxy *)proxy_;
	
	proxy->closed = true;
	proxy->ticker.join();

	mrudp_close_socket(proxy->socket);
	{
		auto lock = lock_of(proxy->mutex);
		for (auto &[id, connection] : proxy->connections)
			mrudp_close_connection(connection.connection);
	}

	proxy->retain = nullptr;
}

mrudp_error_code_t connect (mrudp_connection_t connection, const mrudp_addr_t *remote, mrudp_proxy_magic_t connectionMagic)
{
	// send the first packet
	std::vector<char> data;
	write(data, (char *)&connectionMagic, sizeof(connectionMagic));
	write(data, (char *)remote, sizeof(*remote));
	
	return mrudp_send(connection, data.data(), (int)data.size(), 1);
}

}

