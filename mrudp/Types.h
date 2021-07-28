#pragma once

#include "mrudp.h"
#include "Config.h"
#include "Base.h"

namespace timprepscius {
namespace mrudp {

typedef u_int8_t VersionID;
typedef uint32_t PacketID;
typedef uint16_t ShortConnectionID;

enum TypeID : uint8_t {
	DATA = 'D',
	DATA_UNRELIABLE = 'U',
	ACK = 'A',
	SYN = 'S',
	SYN_ACK = 'Y',
	CLOSE_READ = 'R',
	CLOSE_WRITE = 'W',
	PROBE = 'P',
	ENCRYPTED_VIA_PUBLIC_KEY = 'X',
	ENCRYPTED_VIA_AES = 'E',
} ;

typedef __uint128_t LongConnectionID;

const size_t MAX_PACKET_SIZE = MRUDP_MAX_PACKET_SIZE;
const VersionID MRUDP_VERSION = 1;

typedef mrudp_addr_t Address;

String toString(const Address &addr);

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
