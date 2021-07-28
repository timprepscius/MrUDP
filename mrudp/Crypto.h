#pragma once

#include "Packet.h"

namespace timprepscius {
namespace mrudp {

struct RSAPublicKey;
struct RSAPrivateKey;
struct AESKey;
struct SecureRandom;

struct HostCrypto
{
	StrongPtr<SecureRandom> random;
	StrongPtr<RSAPrivateKey> localRSAPrivateKey;
	StrongPtr<RSAPublicKey> localRSAPublicKey;
	
	HostCrypto();
} ;

struct ConnectionCrypto
{
	StrongPtr<HostCrypto> host;

	StrongPtr<RSAPublicKey> remoteRSAPublicKey;
	StrongPtr<AESKey> localAESKey;
	StrongPtr<AESKey> remoteAESKey;

	ConnectionCrypto(const StrongPtr<HostCrypto> &host);

	bool canEncrypt ();
	bool canDecrypt ();

	bool decrypt (Packet &packet);
	bool encrypt (Packet &packet);
} ;


} // namespace
} // namespace
