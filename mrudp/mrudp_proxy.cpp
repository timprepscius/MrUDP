#include "mrudp_proxy.hpp"
#include "Proxy.hpp"

using namespace timprepscius::mrudp;

void mrudp_proxy_close(void *proxy)
{
	return proxy::close(proxy);
}

void *mrudp_proxy_open(
	mrudp_service_t service,
	const mrudp_addr_t *from,
	const mrudp_addr_t *on,
	const mrudp_addr_t *to,
	const mrudp_proxy_options_t *options,
	mrudp_addr_t *from_bound, mrudp_addr_t *on_bound
)
{
	return proxy::open(service, from, on, to, options, from_bound, on_bound);
}

mrudp_error_code_t mrudp_proxy_connect(mrudp_connection_t connection, const mrudp_addr_t *remote, mrudp_proxy_magic_t magic)
{
	return proxy::connect(connection, remote, magic);
}

mrudp_connection_t mrudp_connect_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *remote,
	
	void *userData,
	mrudp_receive_callback_fn on_receive,
	mrudp_close_callback_fn on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	return mrudp_connect_ex_proxy(
		socket, remote, nullptr,
		userData, on_receive, on_close,
		proxy, magic
	);
}

mrudp_connection_t mrudp_connect_ex_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *remote,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback_fn on_receive,
	mrudp_close_callback_fn on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	auto connection = mrudp_connect_ex(socket, proxy, options, userData, on_receive, on_close);
	
	if (!connection)
		return nullptr;
		
	if (mrudp_failed(mrudp_proxy_connect(connection, remote, magic)))
	{
		mrudp_close_connection(connection);
		return nullptr;
	}
	
	return connection;
}

mrudp_connection_t mrudp_connect_proxy_resolve(
	mrudp_socket_t socket,
	const char *remote_,
	
	void *userData,
	mrudp_receive_callback_fn on_receive,
	mrudp_close_callback_fn on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	mrudp_addr_t remote;
	mrudp_str_to_addr(remote_, &remote);
	
	return mrudp_connect_proxy(
		socket, &remote,
		userData, on_receive, on_close,
		proxy, magic
	);
}

mrudp_connection_t mrudp_connect_ex_proxy_resolve(
	mrudp_socket_t socket,
	const char *remote_,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback_fn on_receive,
	mrudp_close_callback_fn on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	mrudp_addr_t remote;
	mrudp_str_to_addr(remote_, &remote);
	
	return mrudp_connect_ex_proxy(
		socket, &remote, options,
		userData, on_receive, on_close,
		proxy, magic
	);
}

// ------

mrudp_connection_t mrudp_connect_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *remote,
	
	void *userData,
	mrudp_receive_callback &&on_receive,
	mrudp_close_callback &&on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	return mrudp_connect_ex_proxy(
		socket, remote, nullptr,
		userData, std::move(on_receive), std::move(on_close),
		proxy, magic
	);
}

mrudp_connection_t mrudp_connect_ex_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *remote,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback &&on_receive,
	mrudp_close_callback &&on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	auto connection = mrudp_connect_ex(socket, proxy, options, userData, std::move(on_receive), std::move(on_close));
	
	if (!connection)
		return nullptr;
		
	if (mrudp_failed(mrudp_proxy_connect(connection, remote, magic)))
	{
		mrudp_close_connection(connection);
		return nullptr;
	}
	
	return connection;
}

mrudp_connection_t mrudp_connect_proxy_resolve(
	mrudp_socket_t socket,
	const char *remote_,
	
	void *userData,
	mrudp_receive_callback &&on_receive,
	mrudp_close_callback &&on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	mrudp_addr_t remote;
	mrudp_str_to_addr(remote_, &remote);
	
	return mrudp_connect_proxy(
		socket, &remote,
		userData, std::move(on_receive), std::move(on_close),
		proxy, magic
	);
}

mrudp_connection_t mrudp_connect_ex_proxy_resolve(
	mrudp_socket_t socket,
	const char *remote_,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback &&on_receive,
	mrudp_close_callback &&on_close,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
)
{
	mrudp_addr_t remote;
	mrudp_str_to_addr(remote_, &remote);
	
	return mrudp_connect_ex_proxy(
		socket, &remote, options,
		userData, std::move(on_receive), std::move(on_close),
		proxy, magic
	);
}

// ------

mrudp_proxy_options_t mrudp_proxy_options_default()
{
	return proxy::options_default();
}
