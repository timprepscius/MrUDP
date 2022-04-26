#pragma once

#include "mrudp.h"
#include <functional>

typedef std::function<mrudp_error_code_t(void *userData, mrudp_event_t)> mrudp_close_callback;
typedef std::function<mrudp_error_code_t(void *userData, mrudp_connection_t connection)> mrudp_accept_callback;
typedef std::function<mrudp_error_code_t(void *userData, char *data, int size, int is_reliable)> mrudp_receive_callback;
typedef std::function<mrudp_error_code_t(void *userData, const mrudp_addr_t *addresses, size_t numAddresses)> mrudp_resolve_callback;

mrudp_error_code_t mrudp_resolve(mrudp_service_t mrudp, const char *address, mrudp_resolve_callback &&);
 
mrudp_error_code_t mrudp_listen(
	mrudp_socket_t socket,
	void *userData,
	mrudp_accept_callback &&,
	mrudp_close_callback &&
) ;

mrudp_error_code_t mrudp_accept(
	mrudp_connection_t connection,
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&
);

mrudp_error_code_t mrudp_accept_ex(
	mrudp_connection_t connection,
	const mrudp_connection_options_t *options,
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&
);

mrudp_connection_t mrudp_connect(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&
);

mrudp_connection_t mrudp_connect_ex(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	const mrudp_connection_options_t *options,
	
	void *userData,
	mrudp_receive_callback &&,
	mrudp_close_callback &&
);

mrudp_error_code_t mrudp_resolve(mrudp_service_t mrudp, const char *address, mrudp_resolve_callback &&, void *userData);
