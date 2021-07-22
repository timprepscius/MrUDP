#pragma once

#include "Types.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// Header
//
// Header contains the necessary information for the decoding a packet.
// The header is packed so that there is no space between data fields.
// --------------------------------------------------------------------------------
struct Header {
	VersionID version = MRUDP_VERSION;
	ShortConnectionID connection = 0;
	TypeID type;
	PacketID id;
} __attribute__ ((packed));

// --------------------------------------------------------------------------------
// Packet
//
// Packet contains a header area, a data array, and a size field.  The data array's
// memory size is fixed to the maximum packet size.
//
// When a packet is read from the socket, the packet is read verbatim as a chunk
// and the field dataSize is set to sizeof(IncomingPacketData) - sizeof(Header).
// --------------------------------------------------------------------------------
struct Packet
{
	Header header;
	char data[MAX_PACKET_SIZE];
	uint16_t dataSize = 0;
} __attribute__ ((packed));

typedef StrongPtr<Packet> PacketPtr;

// returns whether or not the lhs is greater than the rhs
// when the packet number wraps around (u32), packet id is greater will still
// function correctly
bool packet_id_greater_than(PacketID lhs, PacketID rhs);

template<typename T>
bool popData(Packet &packet, T &t);

template<typename T>
bool peekData(Packet &packet, T &t);

template<typename T>
bool pushData(Packet &packet, const T &);

} // namespace
} // namespace

#include "Packet.inl"
