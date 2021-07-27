#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {
namespace crypto {


// --------------------------------------------------------------------------------
// Sender
//
// --------------------------------------------------------------------------------

struct Sender
{
	mrudp::Sender *sender;


	
	bool canSend ();
	
	void onPacket(Packet &packet);
};

} // namespace
} // namespace
