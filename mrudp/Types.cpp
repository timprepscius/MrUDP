#include "Types.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>

namespace timprepscius {
namespace mrudp {

String toString(const Address &addr)
{
	const size_t MAX_ADDR_LENGTH = 128;
	char str[MAX_ADDR_LENGTH];
	mrudp_addr_to_str(&addr, str, MAX_ADDR_LENGTH);
	
	return str;
}

LongConnectionID copyLongConnectionIDFrom(const char *bytes)
{
	LongConnectionID id;
	memcpy(&id, bytes, sizeof(id));
	
	return id;
}

size_t copyLongConnectionIDTo(char *bytes, LongConnectionID id)
{
	memcpy(bytes, &id, sizeof(id));
	return sizeof(id);
}

void trace_char_(char c)
{
	std::cout << c;
}


void trace_char_(const String &c)
{
	std::cout << c;
}

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}
