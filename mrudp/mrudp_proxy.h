#pragma once

#include "mrudp.h"

typedef uint64_t mrudp_proxy_magic_t;

struct mrudp_proxy_options {
	mrudp_proxy_magic_t magic_wire;
	mrudp_proxy_magic_t magic_connection;
	uint16_t tick_interval_ms;
	uint16_t maximum_wire_retry_attempts;
	uint8_t compression_level;
} ;

typedef mrudp_proxy_options mrudp_proxy_options_t;

mrudp_proxy_options_t mrudp_proxy_options_default();

void *mrudp_proxy_open(
	mrudp_service_t service,
	const mrudp_addr_t *from,
	const mrudp_addr_t *on, // can be nullptr if no wire socket
	const mrudp_addr_t *to,
	const mrudp_proxy_options_t *options,

	mrudp_addr_t *from_bound,
	mrudp_addr_t *to_bound
);

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

mrudp_connection_t mrudp_connect_proxy_resolve(
	mrudp_socket_t socket,
	const char *,
	
	void *userData,
	mrudp_receive_callback_fn,
	mrudp_close_callback_fn,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);

mrudp_connection_t mrudp_connect_ex_proxy_resolve(
	mrudp_socket_t socket,
	const char *,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback_fn,
	mrudp_close_callback_fn,
	
	const mrudp_addr_t *proxy,
	mrudp_proxy_magic_t magic
);
