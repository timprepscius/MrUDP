#include "Crypto.h"
#include "imp/Crypto.h"

namespace timprepscius {
namespace mrudp {

HostCrypto::HostCrypto()
{
	random = strong<SecureRandom>();
	auto keyPair = generateRSAPrivatePublicKeyPair(*random);
	privateKey = keyPair.private_;
	publicKey = keyPair.public_;
}

ConnectionCrypto::ConnectionCrypto(const StrongPtr<HostCrypto> &host_) :
	host(host_)
{
	localSessionKey = generateAESKey(*host->random);
}

bool ConnectionCrypto::canEncrypt ()
{
	return (bool)remoteSessionKey;
}
	
bool ConnectionCrypto::onReceive (Packet &packet)
{
	auto &type = packet.header.type;
	
	if (type == ENCRYPTED_VIA_PUBLIC_KEY)
	{
		if (!host->privateKey || !host->privateKey->decrypt(packet))
			return false;
	}
	else
	if (type == ENCRYPTED_VIA_AES)
	{
		if (!localSessionKey->decrypt(packet))
			return false;
	}

	if (type == H0_CLIENT_PUBLIC_KEY || type == H1_CLIENT_PUBLIC_KEY)
	{
		RSAPublicKey remotePublicKey_;
		if (!popData(packet, remotePublicKey_))
			return false;
			
		if (!remotePublicKey)
		{
			remotePublicKey = strong<RSAPublicKey>(std::move(remotePublicKey_));
		}
	}
	else
	if (type == H2_SESSION_KEY || type == H3_SESSION_KEY)
	{
		AESKey remoteSessionKey_;
		if (!popData(packet, remoteSessionKey_))
			return false;
			
		if (!remoteSessionKey)
		{
			remoteSessionKey = strong<AESKey>(remoteSessionKey_);
		}
	}
	
	return true;
}

bool ConnectionCrypto::onSend (Packet &packet)
{
	auto &type = packet.header.type;

	if (type == H0_CLIENT_PUBLIC_KEY || type == H1_CLIENT_PUBLIC_KEY)
	{
		if (!pushData(packet, *host->publicKey))
			return false;
	}
	else
	if (type == H2_SESSION_KEY || type == H3_SESSION_KEY)
	{
		if (!pushData(packet, *localSessionKey))
			return false;
	}
	
	if (remoteSessionKey)
	{
		return remoteSessionKey->encrypt(packet, MAX_PACKET_POST_CRYPTO_SIZE, *host->random);
	}
	else
	if (remotePublicKey)
	{
		return remotePublicKey->encrypt(packet, MAX_PACKET_POST_CRYPTO_SIZE, *host->random);
	}
	
	return true;
}

} // namespace
} // namespace
