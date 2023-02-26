#pragma once

#include "../mrudp_proxy.h"

namespace timprepscius {
namespace mrudp {
namespace proxy {

void *open(const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound, mrudp_proxy_magic_t);
void close(void *proxy);

void send_connect(mrudp_socket_t socket, const mrudp_addr_t *to, mrudp_proxy_magic_t magic);

} // namespace
} // namespace
} // namespace


