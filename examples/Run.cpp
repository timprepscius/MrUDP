#include "Echo.h"

#include <algorithm>
#include <vector>
#include <map>
#include <string>

std::map<std::string, std::vector<const char *>> parseArgs(int argc, const char* argv[])
{
	std::map<std::string, std::vector<const char *>> result;
	
	std::string prefix = "--";
	std::string lastKey;
	for (auto i=1; i<argc; ++i)
	{
		std::string arg = argv[i];
		if (arg.substr(0, std::min(prefix.size(), arg.size())) == prefix)
		{
			lastKey = arg.substr(prefix.size());
			result[lastKey] = { argv[0] };
		}
		else
		{
			result[lastKey].push_back(argv[i]);
		}
	}
	
	return result;
}

template<typename U, typename T, typename F>
void with_if_not_equal(const U &u, const T &t, F &&f)
{
	if (u != t)
	{
		f(u);
	}
}

int main( int argc, const char* argv[])
{
	auto args = parseArgs(argc, argv);
	
	int result = 0;
	
	with_if_not_equal(
		args.find("run_echo_server_c"), args.end(),
		[&](auto &i) {
			result = run_echo_server_c((int)i->second.size(), i->second.data());
		}
	);

	with_if_not_equal(
		args.find("run_echo_server_cpp"), args.end(),
		[&](auto &i) {
			result = run_echo_server_cpp((int)i->second.size(), i->second.data());
		}
	);

	with_if_not_equal(
		args.find("run_echo_client_c"), args.end(),
		[&](auto &i) {
			result = run_echo_client_c((int)i->second.size(), i->second.data());
		}
	);

	with_if_not_equal(
		args.find("run_echo_client_cpp"), args.end(),
		[&](auto &i) {
			result = run_echo_client_cpp((int)i->second.size(), i->second.data());
		}
	);

	return result;
}
