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
	StrongPtr<RSAPrivateKey> privateKey;
	StrongPtr<RSAPublicKey> publicKey;
	
	HostCrypto();
} ;

struct ConnectionCrypto
{
	StrongPtr<HostCrypto> host;

	StrongPtr<RSAPublicKey> remotePublicKey;
	StrongPtr<AESKey> localSessionKey, remoteSessionKey;

	ConnectionCrypto(const StrongPtr<HostCrypto> &host);

	const TypeID H0_CLIENT_PUBLIC_KEY = H0;
	const TypeID H1_CLIENT_PUBLIC_KEY = H1;
	const TypeID H2_SESSION_KEY = H2;
	const TypeID H3_SESSION_KEY = H3;

	bool canEncrypt ();
	bool canDecrypt ();

	bool onReceive (Packet &packet);
	bool onSend (Packet &packet);
} ;


} // namespace
} // namespace
