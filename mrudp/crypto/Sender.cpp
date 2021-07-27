#pragma once

#include "Crypto.h"

namespace timprepscius {
namespace mrudp {

struct Sender;

namespace crypto {


// --------------------------------------------------------------------------------
// Sender
//
// --------------------------------------------------------------------------------

struct Sender
{
	mrudp::Sender *sender;

	StrongPtr<Encryptor> encryptor;
	
	bool canSend ();
	
	void onPacket(Packet &packet);
};

} // namespace
} // namespace
} // namespace
