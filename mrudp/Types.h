#pragma once

#include "mrudp.h"
#include "Config.h"
#include "Base.h"

namespace timprepscius {
namespace mrudp {

typedef u_int8_t VersionID;
typedef uint16_t ShortConnectionID;

const VersionID MRUDP_VERSION = 1;

enum Reliability {
	UNRELIABLE,
	RELIABLE
} ;

enum ErrorCode {
	OK,
	ERROR_GENERAL_FAILURE = MRUDP_ERROR_GENERAL_FAILURE,
	ERROR_PACKET_SIZE_TOO_LARGE = MRUDP_ERROR_PACKET_SIZE_TOO_LARGE,
	ERROR_CONNECTION_CLOSED = MRUDP_ERROR_PACKET_SIZE_TOO_LARGE,
} ;

typedef __uint128_t LongConnectionID;



using Address = mrudp_addr_t;
using ConnectionOptions = mrudp_connection_options_t;

ConnectionOptions merge(const ConnectionOptions &lhs, const ConnectionOptions &rhs);

String toString(const Address &addr);

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
bool operator==(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
bool operator!=(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
