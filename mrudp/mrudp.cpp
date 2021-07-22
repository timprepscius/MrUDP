#include "mrudp.h"
#include "Socket.h"
#include "Connection.h"
#include "Implementation.h"

#include <iostream>

using namespace timprepscius;
using namespace mrudp;
	
mrudp_socket_t mrudp_socket(
	mrudp_service_t service_,
	mrudp_addr_t *address
)
{
	auto service = toNative(service_);

	auto socket = strong<Socket>(service);
	socket->open(*address);
	
	auto handle = newHandle(socket);
	return (mrudp_socket_t)handle;
}

mrudp_error_code_t mrudp_socket_connect(
	mrudp_socket_t socket_,
	mrudp_addr_t *remote
)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_GENERAL_FAILURE;

	socket->imp->connect(*remote);
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_listen(
	mrudp_socket_t socket_,
	void *userData,
	mrudp_accept_callback_fn acceptCallback,
	mrudp_close_callback_fn closeCallback
)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_GENERAL_FAILURE;
	
	socket->listen(userData, acceptCallback, closeCallback);
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_connection(mrudp_connection_t connection_)
{
	auto connection = closeHandle(connection_);
	if (!connection)
		return MRUDP_GENERAL_FAILURE;
		
	xLogDebug(logVar(connection));

	connection->close();
	connection->setHandlers(nullptr, nullptr, nullptr);
	
	deleteHandle((ConnectionHandle)connection_);

	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_connection_socket_native(mrudp_connection_t connection_)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_GENERAL_FAILURE;
	
	auto socket = connection->socket;
	mrudp_close_connection(connection_);
	
	socket->imp->close();
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_socket_addr (mrudp_socket_t socket_, mrudp_addr_t *address)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_GENERAL_FAILURE;

	xLogDebug(logVar(socket));

	*address = socket->getLocalAddress();
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_connection_remote_addr (mrudp_connection_t connection_, mrudp_addr_t *address)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_GENERAL_FAILURE;

	*address = connection->remoteAddress;
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_socket(mrudp_socket_t socket_)
{
	auto socket = closeHandle(socket_);
	if (!socket)
		return MRUDP_GENERAL_FAILURE;

	socket->close();
	deleteHandle(socket_);
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_socket_native(mrudp_socket_t socket_)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_GENERAL_FAILURE;

	socket->imp->close();
		
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_accept(
	mrudp_connection_t connection_,
	void *userData,
	mrudp_receive_callback_fn receiveHandler,
	mrudp_close_callback_fn closeHandler
)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_GENERAL_FAILURE;

	connection->setHandlers(userData, receiveHandler, closeHandler);
	
	return MRUDP_OK;
}

mrudp_connection_t mrudp_connect(
	mrudp_socket_t socket_,
	const mrudp_addr_t *remoteAddress,
	
	void *userData,
	mrudp_receive_callback_fn receiveCallback,
	mrudp_close_callback_fn eventCallback
)
{
	auto socket = toNative(socket_);
	if (!socket)
		return nullptr;

	xLogDebug(logVar(socket) << logVar(userData));
	
	auto connection = socket->connect(*remoteAddress, userData, receiveCallback, eventCallback);
	
	return newHandle(connection);
}

mrudp_error_code_t mrudp_send(mrudp_connection_t connection_, const char *buffer, int size, int reliable)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_GENERAL_FAILURE;

	if(size < 0 || size > MRUDP_MAX_PACKET_SIZE)
	{
		fprintf(stderr, "mrudp_sendto Error: Attempting to send with invalid max packet size\n");
		return MRUDP_GENERAL_FAILURE;
	}

	xLogDebug(logVar(connection));

	connection->send(buffer, size, reliable);
	
	return MRUDP_OK;
}

mrudp_service_t mrudp_service()
{
	auto service = strong<Service>();
	service->open ();
	
	auto handle = newHandle(service);
	return (mrudp_service_t)handle;
}

void mrudp_close_service(mrudp_service_t service_, int waitForFinish)
{
	auto service = closeHandle(service_);
	if (!service)
		return;

	Service::Close close;

	if (waitForFinish)
	{
		service->close = &close;
	}
	
	service = nullptr;
	deleteHandle(service_);
	
	if (waitForFinish)
	{
		auto lock = std::unique_lock<Mutex>(close.mutex);
		while (!close.value)
			close.event.wait(lock);
	}
}

mrudp_error_code_t mrudp_resolve(mrudp_service_t service_, const char *address, mrudp_resolve_callback_fn callback, void *userData)
{
	auto service = toNative(service_);
	if (!service)
		return MRUDP_GENERAL_FAILURE;
	
	String id(address);
	auto colon = id.rfind(':');
	auto port = id.substr(colon+1);
	auto ip = id.substr(0, colon);
	
	service->imp->resolve(
		ip, port,
		[callback, userData](auto &&addresses) {
			callback(userData, addresses.data(), addresses.size());
		}
	);
	
	return MRUDP_OK;
}


mrudp_error_code_t mrudp_str_to_addr(const char *str, mrudp_addr_t *addr)
{
	memset((char *)addr, 0, sizeof(*addr));
	
	String id(str);
	auto colon = id.rfind(':');
	auto port = id.substr(colon+1);
	auto ip = id.substr(0, colon);
	
	if (ip.find(':')!=-1 || ip.find('[') != -1)
	{
		#ifndef SYS_LINUX
			addr->v6.sin6_len = sizeof(addr->v6);
		#endif
		addr->v6.sin6_family = AF_INET6;

		inet_pton(AF_INET6, ip.c_str(), &(addr->v6.sin6_addr));
		addr->v6.sin6_port = htons(atoi(port.c_str()));
	}
	else
	{
		#ifndef SYS_LINUX
			addr->v4.sin_len = sizeof(addr->v4);
		#endif

		addr->v4.sin_family = AF_INET;

		inet_pton(AF_INET, ip.c_str(), &(addr->v4.sin_addr));
		addr->v4.sin_port = htons(atoi(port.c_str()));
	}
	
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_addr_to_str(const mrudp_addr_t *addr, char *out, size_t outSize)
{
	char str[INET_ADDRSTRLEN];
	int port = 0;
	
	if (addr->v4.sin_family == AF_INET)
	{
		inet_ntop(addr->v4.sin_family, &(addr->v4.sin_addr), str, INET_ADDRSTRLEN);
		port = ntohs(addr->v4.sin_port);
	}
	else
	{
		inet_ntop(addr->v6.sin6_family, &(addr->v6.sin6_addr), str, INET_ADDRSTRLEN);
		port = ntohs(addr->v6.sin6_port);
	}
	
	std::ostringstream ss;
	ss << str << ":" << port;
	auto s = ss.str();
	
	if (s.size() >= outSize)
		return MRUDP_GENERAL_FAILURE;
		
	strcpy(out, s.c_str());

	return MRUDP_OK;
}
