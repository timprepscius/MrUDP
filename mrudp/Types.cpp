#include "Types.h"

#include <cstring>

namespace timprepscius {
namespace mrudp {

String toString(const Address &addr)
{
	const size_t MAX_ADDR_LENGTH = 128;
	char str[MAX_ADDR_LENGTH];
	mrudp_addr_to_str(&addr, str, MAX_ADDR_LENGTH);
	
	return str;
}

ConnectionOptions merge(const ConnectionOptions &lhs, const ConnectionOptions &rhs)
{
	ConnectionOptions merged = lhs;
	
	if (merged.coalesce_reliable.mode == -1)
		merged.coalesce_reliable.mode = rhs.coalesce_reliable.mode;

	if (merged.coalesce_unreliable.mode == -1)
		merged.coalesce_unreliable.mode = rhs.coalesce_unreliable.mode;

	if (merged.coalesce_reliable.delay_ms == -1)
		merged.coalesce_reliable.delay_ms = rhs.coalesce_reliable.delay_ms;

	if (merged.coalesce_unreliable.delay_ms == -1)
		merged.coalesce_unreliable.delay_ms = rhs.coalesce_unreliable.delay_ms;

	return merged;
}

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

bool operator==(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

bool operator!=(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) != 0;
}

