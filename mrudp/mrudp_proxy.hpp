#pragma once

#include "mrudp.hpp"
#include "mrudp_proxy.h"

mrudp_connection_t mrudp_connect_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);

mrudp_connection_t mrudp_connect_ex_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);

mrudp_connection_t mrudp_connect_proxy_resolve(
	mrudp_socket_t socket,
	const char *,
	
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);

mrudp_connection_t mrudp_connect_ex_proxy_resolve(
	mrudp_socket_t socket,
	const char *,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);
