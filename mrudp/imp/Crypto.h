#pragma once

#include "../Packet.h"
#include <random>

namespace timprepscius {
namespace mrudp {
namespace imp {

struct SecureRandom
{
	struct I;
	I *i;
	
	SecureRandom(const SecureRandom &) = delete;

	SecureRandom();
	~SecureRandom();
	
	bool generate(u8 *, size_t size);
} ;

template<int N>
struct AESKey;

template<int N>
struct RSAKey
{
	static constexpr int BitSize = N;

	struct I;
	I *i = nullptr;

	RSAKey(const RSAKey &) = delete;
	
	RSAKey(RSAKey &&v)
	{
		std::swap(i, v.i);
	}
	
	RSAKey () {}
	~RSAKey ();

	bool encrypt(Packet &packet, size_t maxPadTo, SecureRandom &random);
	bool decrypt(Packet &packet);
	
	template<int KN>
	bool encrypt(const AESKey<KN> &in, Vector<u8> &out, SecureRandom &random);

	template<int KN>
	bool decrypt(const Vector<u8> &in, AESKey<KN> &out);
} ;

template<int N>
struct AESIV
{
	static constexpr int BitSize = N;
	static constexpr int ByteSize = BitSize / 8;

	u8 data[ByteSize];
} ;

template<int N>
struct AESKey
{
	static constexpr int BitSize = N;
	static constexpr int ByteSize = BitSize / 8;
	
	u8 data[ByteSize];
	
	bool encrypt(Packet &packet, size_t maxPadTo, SecureRandom &random);
	bool decrypt(Packet &packet);
} ;

template<int N>
struct SHAKey
{
	static constexpr int BitSize = N;
	static constexpr int ByteSize = BitSize / 8;
	
	u8 data[ByteSize];
	
	bool sign(u8 *data, size_t size, u8 *signature, size_t signatureSize);
	bool verify(u8 *data, size_t size, u8 *signature, size_t signatureSize);
} ;

constexpr int DefaultRSAKeySize = 1024;
constexpr int DefaultAESKeySize = 256;
constexpr int DefaultSHAKeySize = 256;

using RSAKeyDefault = RSAKey<DefaultRSAKeySize>;
using AESKeyDefault = AESKey<DefaultAESKeySize>;
using AESIVDefault = AESIV<DefaultAESKeySize>;
using SHAKeyDefault = SHAKey<DefaultSHAKeySize>;

} // namespace

struct RSAPublicKey : imp::RSAKeyDefault {};
struct RSAPrivateKey : imp::RSAKeyDefault {};
struct AESKey : imp::AESKeyDefault {};
struct SHAKey : imp::SHAKeyDefault {};
struct SecureRandom : imp::SecureRandom {};

template<>
bool pushData(Packet &packet, const RSAPublicKey &publicKey);

template<>
bool popData(Packet &packet, RSAPublicKey &publicKey);

struct RSAPrivatePublicKeyPair
{
	StrongPtr<RSAPrivateKey> private_;
	StrongPtr<RSAPublicKey> public_;
} ;

RSAPrivatePublicKeyPair generateRSAPrivatePublicKeyPair(SecureRandom &random);
StrongPtr<AESKey> generateAESKey (SecureRandom &random);
StrongPtr<SHAKey> generateSHAKey (SecureRandom &random);

} // namespace
} // namespace

#include "Crypto.inl"
