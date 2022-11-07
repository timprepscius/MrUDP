#include "../mrudp/mrudp.h"
#include "../mrudp/mrudp_proxy.h"

#include <iostream>
#include <thread>
#include <chrono>

void usage ()
{
	std::cout << "proxy LOCAL remote=<REMOTE> magic=<MAGIC>" << std::endl;
}

std::string_view get_arg(const std::string_view &key, const std::string_view &arg)
{
	if (key.size() > arg.size())
		return {};
		
	if (arg.substr(0, key.size()) != key)
		return {};
	
	return arg.substr(key.size());
}

int main (int argc, const char *argv[])
{
	mrudp_addr_t from, to;
	if (mrudp_str_to_addr(argv[1], &from) != MRUDP_OK)
		return (void)usage(), -1;
	
	mrudp_addr_t *to_ = nullptr;
	mrudp_proxy_magic_t magic = 0;
	for (auto i=2;i<argc;++i)
	{
		auto remote_ = get_arg("remote=", argv[i]);
		auto magic_ = get_arg("magic=", argv[i]);
		if (!remote_.empty())
		{
			if (mrudp_str_to_addr(remote_.data(), &to) != MRUDP_OK)
				return (void)usage(), -1;
		
			to_ = &to;
		}
		else
		if (!magic_.empty())
		{
			magic = atoll(magic_.data());
		}
	}
	
	mrudp_addr_t bound;
	auto *proxy = mrudp_proxy_open(&from, to_, &bound, magic);
	
	while (true)
		std::this_thread::sleep_for(std::chrono::seconds(1));
	
	// proxy_close(proxy);
	
	return 0;
}
