#include "../mrudp/mrudp.h"
#include "../mrudp/mrudp_proxy.h"

#include "../mrudp/base/Core.h"

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
	xLogInitialize("mrudp-proxy.log");
	xLogActivateStory("mrudp::proxy::detail");
	xLogActivateStory("mrudp::proxy");
	xLogActivateStory("mrudp::proxy::compress");
	
	mrudp_addr_t from, to;
	if (mrudp_str_to_addr(argv[1], &from) != MRUDP_OK)
		return (void)usage(), -1;
	
	mrudp_proxy_options_t options = mrudp_proxy_options_default();
	
	mrudp_addr_t *to_ = nullptr;
	for (auto i=2;i<argc;++i)
	{
		auto remote_ = get_arg("remote=", argv[i]);
		auto wireMagic_ = get_arg("wireMagic=", argv[i]);
		auto connectionMagic_ = get_arg("connectionMagic=", argv[i]);
		auto compressionLevel_ = get_arg("compressionLevel=", argv[i]);
		auto tickIntervalMS_ = get_arg("tickIntervalMS=", argv[i]);
		if (!remote_.empty())
		{
			if (mrudp_str_to_addr(remote_.data(), &to) != MRUDP_OK)
				return (void)usage(), -1;
		
			to_ = &to;
		}
		else
		if (!wireMagic_.empty())
		{
			options.magic_wire = atoll(wireMagic_.data());
		}
		else
		if (!connectionMagic_.empty())
		{
			options.magic_connection = atoll(connectionMagic_.data());
		}
		else
		if (!compressionLevel_.empty())
		{
			options.compression_level = atoll(compressionLevel_.data());
		}
		else
		if (!tickIntervalMS_.empty())
		{
			options.tick_interval_ms = atoll(tickIntervalMS_.data());
		}
	}
	
	auto service = mrudp_service();
	
	mrudp_addr_t bound;
	auto *proxy = mrudp_proxy_open(service, &from, to_, &bound, &options);
	
	while (true)
		std::this_thread::sleep_for(std::chrono::seconds(1));
	
	// proxy_close(proxy);
	
	return 0;
}
