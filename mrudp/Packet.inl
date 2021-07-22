#include <cstring>

namespace timprepscius {
namespace mrudp {

inline
bool packet_id_greater_than(PacketID lhs, PacketID rhs)
{
	static const PacketID top_bit = (1 << (sizeof(PacketID) * 8 - 1));

	auto lhs_gtr_rhs = lhs > rhs;
	auto diff = lhs_gtr_rhs ? (lhs - rhs) : (rhs - lhs);
		
	// is the difference between the two greater than the top_bit?
	if (diff > top_bit)
	{
		// then we reverse the greater than
		// this probably should be done with some xor operator or something
		return !lhs_gtr_rhs;
	}

	return lhs_gtr_rhs;
}

template<typename T>
bool popData(Packet &packet, T &t)
{
	if (packet.dataSize < sizeof(T))
		return false;
		
	packet.dataSize -= sizeof(T);
	memcpy(&t, packet.data + packet.dataSize, sizeof(T));

	return true;
}

template<typename T>
bool peekData(Packet &packet, T &t)
{
	if (packet.dataSize < sizeof(T))
		return false;
		
	memcpy(&t, packet.data + packet.dataSize - sizeof(T), sizeof(T));

	return true;
}


template<typename T>
bool pushData(Packet &packet, const T &t)
{
	if (packet.dataSize > MAX_PACKET_SIZE - sizeof(T))
	{
		debug_assert(false);
		return false;
	}
		
	memcpy(packet.data + packet.dataSize, &t, sizeof(LongConnectionID));
	packet.dataSize += sizeof(T);
	
	return true;
}


} // namespace
} // namespace
