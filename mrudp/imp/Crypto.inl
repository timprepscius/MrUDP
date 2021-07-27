#pragma once

#include "../Packet.h"

namespace timprepscius {
namespace mrudp {
namespace crypto {

template<typename N>
struct RSAKey
{
	static const int BitSize = N;
	static const int ByteSize = BitSize / 8;

	u8[ByteSize] key;
	
	void decrypt(Packet &packet);
	void encrypt(Packet &packet);
} ;

template<int N>
struct ShaKey
{
	static const int BitSize = N;
	static const int ByteSize = BitSize / 8;
	
	u8[ByteSize] key;
	
	void decrypt(Packet &packet);
	void encrypt(Packet &packet);
} ;

using PublicKey = RSAKey;
using PrivateKey = RSAKey;
using ShaKey = Sha256Key;

} // namespace
} // namespace
} // namespace
