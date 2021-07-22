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
	PROBE = 'P'
} ;

typedef __uint128_t LongConnectionID;

LongConnectionID copyLongConnectionIDFrom(const char *);
size_t copyLongConnectionIDTo(char *, LongConnectionID);

const size_t MAX_PACKET_SIZE = MRUDP_MAX_PACKET_SIZE;
const VersionID MRUDP_VERSION = 1;

typedef mrudp_addr_t Address;

String toString(const Address &addr);

void trace_char_(char c);
void trace_char_(const String &c);

inline
void trace_char(char c)
{
#ifdef MRUDP_SINGLE_CHAR_TRACE
	trace_char_(c);
#endif
}

inline
void trace_char(const String &c)
{
#ifdef MRUDP_SINGLE_CHAR_TRACE
	trace_char_(c);
#endif
}

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
