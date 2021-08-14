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
	typedef char Data[MAX_PACKET_SIZE];
	typedef uint16_t Size;
	
	Header header;
	Data data;
	Size dataSize = 0;
} __attribute__ ((packed));

typedef StrongPtr<Packet> PacketPtr;

// --------------------------------------------------------------------------------
// DataHeader
//
// Packets with type DATA_RELIABLE or DATA_UNRELIABLE are containers for multiple
// subpackets of data.
// --------------------------------------------------------------------------------
struct DataHeader {
	typedef Packet::Size Size;
	
	DataID id;
	DataTypeID type;
	Size dataSize;
} __attribute__ ((packed));

bool operator==(const Packet &lhs, const Packet &rhs);

// returns whether or not the lhs is greater than the rhs
// when the packet number wraps around (u32), packet id is greater will still
// function correctly
bool packet_id_greater_than(PacketID lhs, PacketID rhs);

bool popData(Packet &packet, u8 *data, size_t size);

template<typename T>
bool popData(Packet &packet, T &t);

template<>
bool popData(Packet &packet, Vector<u8> &data);

template<typename T>
bool peekData(Packet &packet, T &t);

bool pushData(Packet &packet, const u8 *data, size_t size);

bool pushData(Packet &packet, const DataHeader &, const u8 *data);
bool popData(Packet &packet, DataHeader &, u8 *data);

template<typename T>
bool pushData(Packet &packet, const T &);

template<>
bool pushData(Packet &packet, const Vector<u8> &data);

template<typename T>
size_t pushSize(const T &);

template<>
size_t pushSize(const Vector<u8> &data);

} // namespace
} // namespace

#include "Packet.inl"
