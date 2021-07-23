#pragma once

#include "../Types.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// SendIDGenerator
//
// SendIDGenerator generates unique packet IDs for a connection.
// --------------------------------------------------------------------------------

struct SendIDGenerator
{
	Atomic<PacketID> nextID_;
	
	SendIDGenerator ();
	PacketID nextID();
};

} // namespace
} // namespace
