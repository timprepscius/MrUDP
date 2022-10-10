#pragma once

#include "../mrudp.h"

namespace timprepscius {
namespace mrudp {
namespace proxy {

void *open(const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound);
void close(void *proxy);

void send_connect(mrudp_socket_t socket, const mrudp_addr_t *to);

} // namespace
} // namespace
} // namespace


