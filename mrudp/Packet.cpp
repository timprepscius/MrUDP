#include "Packet.h"

namespace timprepscius {
namespace mrudp {

bool operator==(const Packet &lhs, const Packet &rhs)
{
	return
		(lhs.dataSize == rhs.dataSize) &&
		(memcmp(&lhs, &rhs, lhs.dataSize + sizeof(lhs.header)) == 0);
}


} // namespace
} // namespace
