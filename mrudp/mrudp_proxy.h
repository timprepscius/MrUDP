#pragma once

#include "mrudp.h"

typedef uint64_t mrudp_proxy_magic_t;

void *mrudp_proxy_open(mrudp_service_t service, const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound, mrudp_proxy_magic_t wireMagic, mrudp_proxy_magic_t connectionMagic);
void mrudp_proxy_close(void *proxy);

mrudp_error_code_t mrudp_proxy_connect(mrudp_connection_t, const mrudp_addr_t *remote_address, mrudp_proxy_magic_t magic);

mrudp_connection_t mrudp_connect_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	
	void *userData,
	mrudp_receive_callback_fn,
	mrudp_close_callback_fn,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);

mrudp_connection_t mrudp_connect_ex_proxy(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback_fn,
	mrudp_close_callback_fn,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);
