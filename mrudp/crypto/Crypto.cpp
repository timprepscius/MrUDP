#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {
namespace crypto {

struct Crypto
{
	bool canEncrypt ();
	void canDecrypt ();

	void decrypt (Packet &packet);
	void encrypt (Packet &packet);
}

} // namespace
} // namespace
} // namespace
