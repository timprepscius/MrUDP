#include "Crypto.h"
#include "imp/Crypto.h"

namespace timprepscius {
namespace mrudp {

HostCrypto::HostCrypto()
{
	random = strong<SecureRandom>();
	auto keyPair = generateRSAPrivatePublicKeyPair(*random);
	localRSAPrivateKey = keyPair.private_;
	localRSAPublicKey = keyPair.public_;
}

ConnectionCrypto::ConnectionCrypto(const StrongPtr<HostCrypto> &host_) :
	host(host_)
{
	localAESKey = generateAESKey(*host->random);
}

bool ConnectionCrypto::canEncrypt ()
{
	return (bool)remoteAESKey;
}
	
bool ConnectionCrypto::decrypt (Packet &packet)
{
	if (packet.header.type == SYN)
	{
		RSAPublicKey remoteRSAPublicKey_;
		if (!popData(packet, remoteRSAPublicKey_))
			return false;
			
		if (!remoteRSAPublicKey)
		{
			remoteRSAPublicKey = strong<RSAPublicKey>(std::move(remoteRSAPublicKey_));
		}
	}
	else
	{
		if (packet.header.type == ENCRYPTED_VIA_PUBLIC_KEY)
		{
			if (!host->localRSAPrivateKey->decrypt(packet))
				return false;
		}
		else
		if (packet.header.type == ENCRYPTED_VIA_AES)
		{
			if (!localAESKey->decrypt(packet))
				return false;
		}
		else
		{
			return false;
		}

		if (packet.header.type == SYN_ACK)
		{
			AESKey remoteAESKey_;
			if (!popData(packet, remoteAESKey_))
				return false;
				
			if (!remoteAESKey)
			{
				remoteAESKey = strong<AESKey>(remoteAESKey_);
			}
		}
	}
	
	return true;
}

bool ConnectionCrypto::encrypt (Packet &packet)
{
	if (packet.header.type == SYN)
	{
		if (!pushData(packet, *host->localRSAPublicKey))
			return false;
		
		return true;
	}
	else
	{
		if (packet.header.type == SYN_ACK)
		{
			if (!pushData(packet, *localAESKey))
				return false;
		}
	
		if (remoteAESKey)
		{
			return remoteAESKey->encrypt(packet, MAX_PACKET_SIZE - sizeof(LongConnectionID), *host->random);
		}
		else
		if (remoteRSAPublicKey)
		{
			return remoteRSAPublicKey->encrypt(packet, MAX_PACKET_SIZE - sizeof(LongConnectionID), *host->random);
		}
		else
		{
			return false;
		}
	}
}

} // namespace
} // namespace
