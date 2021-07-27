#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {
namespace crypto {

struct PublicKey;
struct PrivateKey;
struct ShaKey;

struct Crypto
{
	StrongPtr<PrivateKey> localPrivateKey;
	StrongPtr<PublicKey> localPublicKey;
	StrongPtr<PublicKey> remotePublicKey;
	
	StrongPtr<ShaKey> localShaKey;
	StrongPtr<ShaKey> remoteShaKey;

	bool canEncrypt ();
	void canDecrypt ();

	void decrypt (Packet &packet);
	void encrypt (Packet &packet);
}

} // namespace
} // namespace
} // namespace
