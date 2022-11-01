#pragma once

#include "mrudp.h"

typedef uint64_t mrudp_proxy_magic_t;

void *mrudp_proxy_open(const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound, mrudp_proxy_magic_t);
void mrudp_proxy_close(void *proxy);

void mrudp_send_proxy_connect(mrudp_socket_t socket, const mrudp_addr_t *to, mrudp_proxy_magic_t);

