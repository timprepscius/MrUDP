
#include "Common.h"
#include "../mrudp/Packet.h"
#include "../mrudp/Crypto.h"
#include "../mrudp/imp/Crypto.h"

namespace timprepscius {
namespace mrudp {
namespace tests {

SCENARIO("mrudp crypto")
{
    GIVEN( "a packet" )
    {
		mrudp::Packet a;
		a.header.connection = (int)'C';
		a.header.id = (int)'I';
		a.header.type = ACK;
		a.header.version = MRUDP_VERSION;
		
		for (auto i=0; i<12; ++i)
			a.data[i] = 'A' + i;
			
		a.dataSize = 12;
			
		mrudp::SecureRandom random;
		auto keys = mrudp::generateRSAPrivatePublicKeyPair(random);
		auto aes = mrudp::generateAESKey(random);
			
		WHEN("encrypting with aes")
		{
			auto b = a;
			aes->encrypt(b, 32, random);
			
			auto c = b;
			aes->decrypt(c);
			
			REQUIRE(c == a);
		}

		WHEN("encrypting with rsa")
		{
			auto b = a;
			keys.public_->encrypt(b, MAX_PACKET_SIZE, random);
			
			auto c = b;
			keys.private_->decrypt(c);
			
			REQUIRE(c == a);
		}
		
		WHEN("encrypting with connection crypto")
		{
			auto hostA = strong<mrudp::HostCrypto>();
			auto hostB = strong<mrudp::HostCrypto>();
			auto connectionA = strong<mrudp::ConnectionCrypto>(hostA);
			auto connectionB = strong<mrudp::ConnectionCrypto>(hostB);

			{
				mrudp::Packet p;
				p.header.type = SYN;
				p.header.connection = 1;
				p.dataSize = 0;
				auto c = p;
				
				REQUIRE(connectionA->encrypt(p));
				REQUIRE(connectionB->decrypt(p));
				REQUIRE(c == p);
			}

			{
				mrudp::Packet p;
				p.header.type = SYN;
				p.header.connection = 1;
				p.dataSize = 0;
				auto c = p;
				
				REQUIRE(connectionB->encrypt(p));
				REQUIRE(connectionA->decrypt(p));
				REQUIRE(c == p);
			}
			
			{
				mrudp::Packet p;
				p.header.type = SYN_ACK;
				p.header.connection = 1;
				p.dataSize = 0;
				auto c = p;
				
				REQUIRE(connectionA->encrypt(p));
				REQUIRE(connectionB->decrypt(p));
				REQUIRE(c == p);
			}

			{
				mrudp::Packet p;
				p.header.type = SYN_ACK;
				p.header.connection = 1;
				p.dataSize = 0;
				auto c = p;
				
				REQUIRE(connectionB->encrypt(p));
				REQUIRE(connectionA->decrypt(p));
				REQUIRE(c == p);
			}
			
			{
				mrudp::Packet p;
				p.header.type = DATA;
				p.header.connection = 1;
				p.data[0] = 'H';
				p.dataSize = 1;
				auto c = p;

				REQUIRE(connectionA->encrypt(p));
				REQUIRE(connectionB->decrypt(p));
				REQUIRE(c == p);
			}

			{
				mrudp::Packet p;
				p.header.type = DATA;
				p.header.connection = 1;
				p.data[0] = 'H';
				p.dataSize = 1;
				auto c =  p;

				REQUIRE(connectionB->encrypt(p));
				REQUIRE(connectionA->decrypt(p));
				REQUIRE(c == p);
			}
		}
	}
}

} // namespace
} // namespace
} // namespace
