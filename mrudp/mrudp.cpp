#include "mrudp.h"
#include "mrudp.hpp"
#include "Socket.h"
#include "Connection.h"
#include "Implementation.h"

#include <iostream>

using namespace timprepscius;
using namespace mrudp;

mrudp_error_code_t mrudp_default_options (mrudp_imp_selector imp, void *options_)
{
	// TODO: size checks
	auto options = imp::getDefaultOptions();
	memcpy(options_, &options, sizeof(options));
	return MRUDP_OK;
}

	
mrudp_socket_t mrudp_socket(
	mrudp_service_t service_,
	const mrudp_addr_t *address
)
{
	auto service = toNative(service_);

	auto socket = strong<Socket>(service);
	socket->open(*address);
	
	auto handle = newHandle(socket);
	return (mrudp_socket_t)handle;
}

mrudp_error_code_t mrudp_listen(
	mrudp_socket_t socket_,
	void *userData,
	mrudp_accept_callback &&acceptCallback,
	mrudp_close_callback &&closeCallback
)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_ERROR_GENERAL_FAILURE;
	
	socket->listen(userData, std::move(acceptCallback), std::move(closeCallback));
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_listen(
	mrudp_socket_t socket_,
	void *userData,
	mrudp_accept_callback_fn acceptCallback,
	mrudp_close_callback_fn closeCallback
)
{
	return mrudp_listen(socket_, userData, mrudp_accept_callback(acceptCallback), mrudp_close_callback(closeCallback));
}

mrudp_error_code_t mrudp_close_connection(mrudp_connection_t connection_)
{
	auto connection = closeHandle(connection_);
	if (!connection)
		return MRUDP_ERROR_GENERAL_FAILURE;
		
	xLogDebug(logVar(connection));

	connection->close();
	connection->closeUser(MRUDP_EVENT_CLOSED);
	
	deleteHandle((ConnectionHandle)connection_);

	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_connection_socket_native(mrudp_connection_t connection_)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_ERROR_GENERAL_FAILURE;
	
	auto socket = connection->socket;
	mrudp_close_connection(connection_);
	
	socket->imp->close();
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_socket_addr (mrudp_socket_t socket_, mrudp_addr_t *address)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_ERROR_GENERAL_FAILURE;

	xLogDebug(logVar(socket));

	*address = socket->getLocalAddress();
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_connection_remote_addr (mrudp_connection_t connection_, mrudp_addr_t *address)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_ERROR_GENERAL_FAILURE;

	*address = connection->remoteAddress;
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_connection_statistics (mrudp_connection_t connection_, mrudp_connection_statistics_t *statistics)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_ERROR_GENERAL_FAILURE;

	*statistics = connection->statistics.query();
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_relocate_socket(mrudp_socket_t socket_, const mrudp_addr_t *address)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_ERROR_GENERAL_FAILURE;

	socket->imp->relocate(*address);

	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_socket(mrudp_socket_t socket_)
{
	auto socket = closeHandle(socket_);
	if (!socket)
		return MRUDP_ERROR_GENERAL_FAILURE;

	socket->close();
	deleteHandle(socket_);
	
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_close_socket_native(mrudp_socket_t socket_)
{
	auto socket = toNative(socket_);
	if (!socket)
		return MRUDP_ERROR_GENERAL_FAILURE;

	socket->imp->close();
		
	return MRUDP_OK;
}

mrudp_error_code_t mrudp_accept(
	mrudp_connection_t connection_,
	void *userData,
	mrudp_receive_callback &&receiveHandler,
	mrudp_close_callback &&closeHandler
)
{
	return mrudp_accept_ex(connection_, nullptr, userData, std::move(receiveHandler), std::move(closeHandler));
}

mrudp_error_code_t mrudp_accept(
	mrudp_connection_t connection_,
	void *userData,
	mrudp_receive_callback_fn receiveHandler,
	mrudp_close_callback_fn closeHandler
)
{
	return mrudp_accept_ex(connection_, nullptr, userData, receiveHandler, closeHandler);
}

mrudp_error_code_t mrudp_accept_ex(
	mrudp_connection_t connection_,
	const mrudp_connection_options_t *options,
	void *userData,
	mrudp_receive_callback &&receiveHandler,
	mrudp_close_callback &&closeHandler
)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_ERROR_GENERAL_FAILURE;

	connection->openUser(options, userData, std::move(receiveHandler), std::move(closeHandler));
	
	return MRUDP_OK;
}


mrudp_error_code_t mrudp_accept_ex(
	mrudp_connection_t connection_,
	const mrudp_connection_options_t *options,
	void *userData,
	mrudp_receive_callback_fn receiveHandler,
	mrudp_close_callback_fn closeHandler
)
{
	return mrudp_accept_ex(connection_, options, userData, mrudp_receive_callback(receiveHandler), mrudp_close_callback(closeHandler));
}

mrudp_connection_t mrudp_connect(
	mrudp_socket_t socket_,
	const mrudp_addr_t *remoteAddress,
	void *userData,
	mrudp_receive_callback_fn receiveCallback,
	mrudp_close_callback_fn eventCallback
)
{
	return mrudp_connect_ex(socket_, remoteAddress, nullptr, userData, receiveCallback, eventCallback);
}

mrudp_connection_t mrudp_connect(
	mrudp_socket_t socket_,
	const mrudp_addr_t *remoteAddress,
	void *userData,
	mrudp_receive_callback &&receiveCallback,
	mrudp_close_callback &&eventCallback
)
{
	return mrudp_connect_ex(socket_, remoteAddress, nullptr, userData, std::move(receiveCallback), std::move(eventCallback));
}

mrudp_connection_t mrudp_connect_ex(
	mrudp_socket_t socket_,
	const mrudp_addr_t *remoteAddress,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback_fn receiveCallback,
	mrudp_close_callback_fn eventCallback
)
{
	return mrudp_connect_ex(socket_, remoteAddress, options, userData, mrudp_receive_callback(receiveCallback), mrudp_close_callback(eventCallback));
}

mrudp_connection_t mrudp_connect_ex(
	mrudp_socket_t socket_,
	const mrudp_addr_t *remoteAddress,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback &&receiveCallback,
	mrudp_close_callback &&eventCallback
)
{
	auto socket = toNative(socket_);
	if (!socket)
		return nullptr;

	xLogDebug(logVar(socket) << logVar(userData));
	
	auto connection = socket->connect(*remoteAddress, options, userData, std::move(receiveCallback), std::move(eventCallback));
	
	if (!connection)
		return nullptr;
		
	return newHandle(connection);
}

mrudp_error_code_t mrudp_send(mrudp_connection_t connection_, const char *buffer, int size, int reliable)
{
	auto connection = toNative(connection_);
	if (!connection)
		return MRUDP_ERROR_GENERAL_FAILURE;

	xLogDebug(logVar(connection));

	return connection->send(buffer, size, reliable == 0 ? UNRELIABLE : RELIABLE);
}

mrudp_service_t mrudp_service()
{
	return mrudp_service_ex(MRUDP_IMP_ASIO, nullptr);
}

mrudp_service_t mrudp_service_ex(mrudp_imp_selector imp, void *options)
{
	auto service = strong<Service>(imp, options);
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
	return mrudp_resolve(service_, address, mrudp_resolve_callback(callback), userData);
}

mrudp_error_code_t mrudp_resolve(mrudp_service_t service_, const char *address, mrudp_resolve_callback &&callback, void *userData)
{
	auto service = toNative(service_);
	if (!service)
		return MRUDP_ERROR_GENERAL_FAILURE;
	
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
		#ifdef SYS_APPLE
			addr->v6.sin6_len = sizeof(addr->v6);
		#endif
		addr->v6.sin6_family = AF_INET6;

		inet_pton(AF_INET6, ip.c_str(), &(addr->v6.sin6_addr));
		addr->v6.sin6_port = htons(atoi(port.c_str()));
	}
	else
	{
		#ifdef SYS_APPLE
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
		return MRUDP_ERROR_GENERAL_FAILURE;
		
	strcpy(out, s.c_str());

	return MRUDP_OK;
}
