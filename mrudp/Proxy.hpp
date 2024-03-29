#pragma once

#include "mrudp.h"
#include "mrudp_proxy.h"

namespace timprepscius::mrudp::proxy {

void *open(
	mrudp_service_t service,
	const mrudp_addr_t *from,
	const mrudp_addr_t *on,
	const mrudp_addr_t *to,
	const mrudp_proxy_options_t *options,
	
	mrudp_addr_t *from_bound,
	mrudp_addr_t *on_bound
);

void close(void *proxy);

mrudp_error_code_t connect(
	mrudp_connection_t connection,
	const mrudp_addr_t *remote,
	mrudp_proxy_magic_t connectionMagic
);

mrudp_proxy_options_t options_default();

}

