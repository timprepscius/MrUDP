#include "mrudp_proxy.h"
#include "proxy/Proxy.h"

using namespace timprepscius::mrudp;

void mrudp_proxy_close(void *proxy)
{
	return proxy::close(proxy);
}

void *mrudp_proxy_open(const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound, mrudp_proxy_magic_t magic)
{
	return proxy::open(from, to, bound, magic);
}

void mrudp_send_proxy_connect(mrudp_socket_t socket, const mrudp_addr_t *to, mrudp_proxy_magic_t magic)
{
	return proxy::send_connect(socket, to, magic);
}
