#include "Types.h"

#include <cstring>
#include <iostream>

namespace timprepscius {
namespace mrudp {

bool operator ==(const LongConnectionID &lhs, const LongConnectionID &rhs)
{
	return memcmp(lhs.bytes, rhs.bytes, sizeof(lhs.bytes)) == 0;
}

bool operator !=(const LongConnectionID &lhs, const LongConnectionID &rhs)
{
	return !(lhs==rhs);
}

const LongConnectionID NullLongConnectionID = LongConnectionID{0};

std::ostream &operator <<(std::ostream &o, const LongConnectionID &v)
{
	for (auto &b : v.bytes)
	{
		u8 n0 = b & 0xF;
		char c0 = ('a' + n0);
		o << c0;
		
		u8 n1 = (b >> 4) & 0xF;
		char c1 = ('a' + n1);
		o << c1;
	}
	
	return o;
}

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
		
	if (merged.probe_delay_ms == -1)
		merged.probe_delay_ms = rhs.probe_delay_ms;

	if (merged.maximum_retry_attempts == -1)
		merged.maximum_retry_attempts = rhs.maximum_retry_attempts;

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
