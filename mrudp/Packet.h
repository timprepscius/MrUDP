#pragma once

#include "Types.h"

namespace timprepscius {
namespace mrudp {

enum TypeID : uint8_t {
	NONE = 0,
	H0 = 'A',
	H1 = 'B',
	H2 = 'C',
	H3 = 'D',
	HANDSHAKE_COMPLETE = 'E',
	
	ACK = 'K',
	DATA_RELIABLE = 'R',
	DATA_UNRELIABLE = 'U',
	PROBE = 'P',
	CLOSE_READ = 'I',

	AUTHENTICATE_CHALLENGE = 'M',
	AUTHENTICATE_RESPONSE = 'W',
	
	ENCRYPTED_VIA_PUBLIC_KEY = 'Z',
	ENCRYPTED_VIA_AES = 'Y',
} ;

inline
bool isHandshake(TypeID typeID)
{
	return typeID >= H0 && typeID < HANDSHAKE_COMPLETE;
}

// TODO:
// these really should be modifications of the typeID for instance typeID & ACK_BIT
// but it is harder to debug, because of not nice printability
// I suppose, I will make a "toPrintable(TypeID)" and use that, but for now not.
inline
bool isAck(TypeID typeID)
{
	return typeID == ACK || typeID == H1 || typeID == H3 || typeID == AUTHENTICATE_RESPONSE;
}

enum FrameTypeID : uint8_t {
	DATA = 'T',
	CLOSE_WRITE = 'W',
} ;

const int MAX_PACKET_SIZE = 1500;
// TODO: put in some static assert somewhere that
// MRUDP_MAX_PACKET_SIZE + sizeof(FrameHeader) + max_crypto_overhead < MAX_PACKET_SIZE

typedef uint16_t PacketID;
typedef uint16_t FrameID;

// --------------------------------------------------------------------------------
// Header
//
// Header contains the necessary information for the decoding a packet.
// The header is packed so that there is no space between data fields.
// --------------------------------------------------------------------------------
PACK (
	struct Header {
		VersionID version = MRUDP_VERSION;
		ShortConnectionID connection = 0;
		TypeID type;
		PacketID id;
	}
);

// --------------------------------------------------------------------------------
// FrameHeader
//
// Packets with type DATA_RELIABLE or DATA_UNRELIABLE are containers for multiple
// subpackets of data.
// --------------------------------------------------------------------------------
PACK (
	struct FrameHeader {
		typedef uint16_t Size;
		
		FrameID id;
		FrameTypeID type;
		Size dataSize;
	}
);
// --------------------------------------------------------------------------------
// Packet
//
// Packet contains a header area, a data array, and a size field.  The data array's
// memory size is fixed to the maximum packet size.
//
// When a packet is read from the socket, the packet is read verbatim as a chunk
// and the field dataSize is set to sizeof(IncomingPacketData) - sizeof(Header).
// --------------------------------------------------------------------------------
PACK(
	struct Packet
	{
		typedef char Data[MAX_PACKET_SIZE];
		typedef uint16_t Size;
		
		Header header;
		Data data;
		Size dataSize = 0;
	}
);

typedef StrongPtr<Packet> PacketPtr;

bool operator==(const Packet &lhs, const Packet &rhs);

struct PacketPath {
	PacketPtr packet;
	Optional<Address> address;
} ;

using MultiPacketPath = StackArray<PacketPath, 2>;

enum PacketDiscard {
	Keep,
	Discard
} ;

// TODO: these constants, especially size constants should be located somewhere else
const int MAX_ROUTE_SIZE = 0;
const int MAX_PACKET_POST_CRYPTO_SIZE = MAX_PACKET_SIZE - MAX_ROUTE_SIZE;
const int MAX_CRYPTO_SIZE = 64; // this should call a function in crypto to find out
const int MAX_PACKET_POST_FRAME_SIZE = MAX_PACKET_POST_CRYPTO_SIZE - MAX_CRYPTO_SIZE;
const int MAX_FRAME_HEADER_SIZE = sizeof(FrameHeader);
const int MAX_PACKET_DATA_SIZE = MAX_PACKET_POST_FRAME_SIZE - MAX_FRAME_HEADER_SIZE;
static_assert(MRUDP_MAX_PACKET_SIZE < MAX_PACKET_DATA_SIZE);

// returns whether or not the lhs is greater than the rhs
// when the packet number wraps around, the id_greater_than will still
// function correctly
template<typename T>
bool id_greater_than(T lhs, T rhs);

bool popData(Packet &packet, u8 *data, size_t size);

template<typename T>
bool popData(Packet &packet, T &t);

template<>
bool popData(Packet &packet, Vector<u8> &data);

template<typename T>
bool peekData(Packet &packet, T &t);

bool pushData(Packet &packet, const u8 *data, size_t size);

bool pushFrame(Packet &packet, const FrameHeader &, const u8 *data);
bool popFrame(Packet &packet, FrameHeader &, u8 *data);

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
