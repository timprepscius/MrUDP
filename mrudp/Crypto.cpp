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

bool ConnectionCrypto::canSend ()
{
	return (bool)remoteSessionKey;
}

bool ConnectionCrypto::canReceive ()
{
	return (bool)localSessionKey;
}
	
PacketDiscard ConnectionCrypto::onReceive (Packet &packet)
{
	auto &type = packet.header.type;
	
	if (type == ENCRYPTED_VIA_PUBLIC_KEY)
	{
		if (!host->privateKey || !host->privateKey->decrypt(packet))
			return Discard;
	}
	else
	if (type == ENCRYPTED_VIA_AES)
	{
		if (!localSessionKey || !localSessionKey->decrypt(packet))
			return Discard;
	}

	if (type == H0_CLIENT_PUBLIC_KEY || type == H1_CLIENT_PUBLIC_KEY)
	{
		RSAPublicKey remotePublicKey_;
		if (!popData(packet, remotePublicKey_))
			return Discard;
			
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
			return Discard;
			
		if (!remoteSessionKey)
		{
			remoteSessionKey = strong<AESKey>(remoteSessionKey_);
		}
	}
	
	return Keep;
}

PacketDiscard ConnectionCrypto::onSend (Packet &packet)
{
	auto &type = packet.header.type;

	if (type == H0_CLIENT_PUBLIC_KEY || type == H1_CLIENT_PUBLIC_KEY)
	{
		if (!pushData(packet, *host->publicKey))
			return Discard;
	}
	else
	if (type == H2_SESSION_KEY || type == H3_SESSION_KEY)
	{
		if (!pushData(packet, *localSessionKey))
			return Discard;
	}
	
	if (remoteSessionKey)
	{
		if (!remoteSessionKey->encrypt(packet, MAX_PACKET_POST_CRYPTO_SIZE, *host->random))
			return Discard;
	}
	else
	if (remotePublicKey)
	{
		if (!remotePublicKey->encrypt(packet, MAX_PACKET_POST_CRYPTO_SIZE, *host->random))
			return Discard;
	}
	
	return Keep;
}

} // namespace
} // namespace
