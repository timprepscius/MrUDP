#pragma once

#include "mrudp.hpp"
#include "Config.h"
#include "Base.h"

namespace timprepscius {
namespace mrudp {

typedef uint8_t VersionID;
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

struct LongConnectionID {
	u8 bytes[16];
} ;

std::ostream &operator <<(std::ostream &, const LongConnectionID &v);

bool operator ==(const LongConnectionID &lhs, const LongConnectionID &rhs);
bool operator !=(const LongConnectionID &lhs, const LongConnectionID &rhs);
extern const LongConnectionID NullLongConnectionID;

using Address = mrudp_addr_t;
using ConnectionOptions = mrudp_connection_options_t;

ConnectionOptions merge(const ConnectionOptions &lhs, const ConnectionOptions &rhs);

String toString(const Address &addr);

} // namespace
} // namespace

bool operator<(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
bool operator==(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);
bool operator!=(const mrudp_addr_t &lhs, const mrudp_addr_t &rhs);

template<>
struct std::hash<timprepscius::mrudp::LongConnectionID> {
    auto operator() (const timprepscius::mrudp::LongConnectionID &key) const {
        std::hash<std::string_view> hasher;
        return hasher(std::string_view((const char *)key.bytes, sizeof(key.bytes)));
    }
};
