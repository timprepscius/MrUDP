#pragma once

#include "mrudp.h"
#include "Config.h"
#include "Base.h"

namespace timprepscius {
namespace mrudp {

typedef u_int8_t VersionID;
typedef uint16_t PacketID;
typedef uint16_t FrameID;
typedef uint16_t ShortConnectionID;

enum Reliability {
	UNRELIABLE,
	RELIABLE
} ;

enum TypeID : uint8_t {
	NONE = 0,
	H0 = 'A',
	H1 = 'B',
	H2 = 'C',
	H3 = 'D',
	HANDSHAKE_COMPLETE = 'E',
	
	ACK = 'K',
	DATA_RELIABLE = 'V',
	DATA_UNRELIABLE = 'Q',
	PROBE = 'P',
	CLOSE_READ = 'R',
	
	ENCRYPTED_VIA_PUBLIC_KEY = 'X',
	ENCRYPTED_VIA_AES = 'Z',
} ;

enum FrameTypeID : uint8_t {
	DATA = 'T',
	CLOSE_WRITE = 'W',
} ;

typedef __uint128_t LongConnectionID;

const size_t MAX_PACKET_SIZE = MRUDP_MAX_PACKET_SIZE;
const VersionID MRUDP_VERSION = 1;

using Address = mrudp_addr_t;
using ConnectionOptions = mrudp_connection_options_t;

ConnectionOptions merge(const ConnectionOptions &lhs, const ConnectionOptions &rhs);

String toString(const Address &addr);

inline
bool isHandshake(TypeID typeID)
{
	return typeID >= H0 && typeID < HANDSHAKE_COMPLETE;
}

inline
bool isAck(TypeID typeID)
{
	return typeID == ACK || typeID == H1 || typeID == H3;
}

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
bool operator==(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
bool operator!=(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
