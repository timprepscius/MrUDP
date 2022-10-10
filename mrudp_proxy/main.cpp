#include "../mrudp/mrudp.h"
#include "../mrudp/mrudp_proxy.h"

#include <iostream>
#include <thread>
#include <chrono>

void usage ()
{
	std::cout << "proxy LOCAL REMOTE" << std::endl;
}

int main (int argc, const char *argv[])
{
	mrudp_addr_t from, to;
	if (mrudp_str_to_addr(argv[1], &from) != MRUDP_OK)
		return (void)usage(), -1;
	
	mrudp_addr_t *to_ = nullptr;
	if (argc > 2)
	{
		if (mrudp_str_to_addr(argv[2], &to) != MRUDP_OK)
			return (void)usage(), -1;
			
		to_ = &to;
	}
	
	mrudp_addr_t bound;
	auto *proxy = mrudp_proxy_open(&from, to_, &bound);
	
	while (true)
		std::this_thread::sleep_for(std::chrono::seconds(1));
	
	// proxy_close(proxy);
	
	return 0;
}
