#include <cstring>

namespace timprepscius {
namespace mrudp {

template<typename T>
inline
bool id_greater_than(T lhs, T rhs)
{
	static const T top_bit = (1 << (sizeof(T) * 8 - 1));

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

inline
bool popData(Packet &packet, u8 *data, size_t size)
{
	if (packet.dataSize < size)
		return false;
		
	packet.dataSize -= size;
	memcpy(data, packet.data + packet.dataSize, size);

	return true;
}

template<typename T>
bool popData(Packet &packet, T &t)
{
	return popData(packet, (u8 *)&t, sizeof(T));
}

template<>
inline
bool popData(Packet &packet, Vector<u8> &t)
{
	u16 size;
	if (!popData(packet, size))
		return false;
		
	t.resize(size);
	return popData(packet, t.data(), t.size());
}

template<typename T>
bool peekData(Packet &packet, T &t)
{
	if (packet.dataSize < sizeof(T))
		return false;
		
	memcpy(&t, packet.data + packet.dataSize - sizeof(T), sizeof(T));

	return true;
}

inline
bool pushData(Packet &packet, const u8 *data, size_t size)
{
	if (packet.dataSize > MAX_PACKET_SIZE - size)
	{
		debug_assert(false);
		return false;
	}
		
	memcpy(packet.data + packet.dataSize, data, size);
	packet.dataSize += size;
	
	return true;
}

template<typename T>
bool pushData(Packet &packet, const T &t)
{
	pushData(packet, (const u8 *)&t, sizeof(T));
	return true;
}

inline
bool pushSizedData(Packet &packet, const u8 *data, size_t size)
{
	return
		pushData(packet, data, size) &&
		pushData(packet, (u16)size) &&
		true;
}

template<>
inline
bool pushData(Packet &packet, const Vector<u8> &data)
{
	return pushSizedData(packet, data.data(), data.size());
}


inline
bool popSizedData(Packet &packet, u8 *data, size_t &size_)
{
	u16 size;
	
	if (!popData(packet, size))
		return false;
		
	size_ = size;
		
	if (!popData(packet, data, size_))
		return false;
		
	return true;
}

template<typename T>
size_t pushSize(const T &t)
{
	return sizeof(T);
}

template<>
inline
size_t pushSize(const Vector<u8> &data)
{
	return data.size() + sizeof(u16);
}

inline
bool pushFrame(Packet &packet, const FrameHeader &header, const u8 *data)
{
	return
		pushData(packet, header) &&
		pushData(packet, data, header.dataSize);
}

inline
bool popFrame(Packet &packet, FrameHeader &header, u8 *data)
{
	return
		popData(packet, header) &&
		popData(packet, data, header.dataSize);
}

} // namespace
} // namespace
