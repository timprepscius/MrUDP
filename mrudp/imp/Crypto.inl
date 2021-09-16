#pragma once

#include "Crypto.h"

namespace timprepscius {
namespace mrudp {
namespace imp {

// --------------------------

//template<int N>
//bool encrypt_(RSAKey<N> &key, Packet &packet, size_t maxPadTo, SecureRandom &random);
//
//template<int N>
//bool decrypt_(RSAKey<N> &key, Packet &packet);
//
//template<int RN, int KN>
//bool encrypt_(RSAKey<RN> &rsa, const AESKey<KN> &in, Vector<u8> &out, SecureRandom &random);
//
//template<int RN, int KN>
//bool decrypt_(RSAKey<RN> &rsa, const Vector<u8> &in, AESKey<KN> &out);
//
//template<int N>
//bool encrypt_(AESKey<N> &key, Packet &packet, size_t maxPadTo, SecureRandom &random);
//
//template<int N>
//bool decrypt_(AESKey<N> &key, Packet &packet);

// --

//template<>
bool encrypt_(RSAKeyDefault &key, Packet &packet, size_t maxPadTo, SecureRandom &random);

//template<>
bool decrypt_(RSAKeyDefault &key, Packet &packet);

//template<>
bool encrypt_(RSAKeyDefault &rsa, const AESKey<DefaultAESKeySize> &in, Vector<u8> &out, SecureRandom &random);

//template<>
bool decrypt_(RSAKeyDefault &rsa, const Vector<u8> &in, AESKey<DefaultAESKeySize> &out);

//template<>
bool encrypt_(AESKeyDefault &key, Packet &packet, size_t maxPadTo, SecureRandom &random);

//template<>
bool decrypt_(AESKeyDefault &key, Packet &packet);

//template<>
bool sign_(SHAKeyDefault &key, u8 *packet, size_t size, u8 *signature, size_t signatureSize);

//template<>
bool verify_(SHAKeyDefault &key, u8 *packet, size_t size, u8 *signature, size_t signatureSize);



// --------------------------

//template<int N>
//bool construct_(SecureRandom &random, AESKey<N> &key);
//
//template<int N>
//bool construct_(SecureRandom &random, RSAKey<N> &private_, RSAKey<N> &public_);
//
//template<int N>
//bool construct_(SecureRandom &random, SHAKey<N> &key);
//
//template<int N>
//void destruct_(RSAKey<N> &key);

// --

//template<>
bool construct_(SecureRandom &random, AESKeyDefault &key);

//template<>
bool construct_(SecureRandom &random, RSAKeyDefault &private_, RSAKeyDefault &public_);

//template<>
void destruct_(RSAKeyDefault &key);


// --------------------------

template<int N>
RSAKey<N>::~RSAKey()
{
	destruct_(*this);
}

template<int N>
bool RSAKey<N>::encrypt(Packet &packet, size_t maxPadTo, SecureRandom &random)
{
	return encrypt_(*this, packet, maxPadTo, random);
}

template<int N>
bool RSAKey<N>::decrypt(Packet &packet)
{
	return decrypt_(*this, packet);
}

template<int N>
template<int KN>
bool RSAKey<N>::encrypt(const AESKey<KN> &in, Vector<u8> &out, SecureRandom &random)
{
	return encrypt_(*this, in, out, random);
}

template<int N>
template<int KN>
bool RSAKey<N>::decrypt(const Vector<u8> &in, AESKey<KN> &out)
{
	return decrypt_(*this, in, out);
}

template<int N>
bool AESKey<N>::encrypt(Packet &packet, size_t maxPadTo, SecureRandom &random)
{
	return encrypt_(*this, packet, maxPadTo, random);
}

template<int N>
bool AESKey<N>::decrypt(Packet &packet)
{
	return decrypt_(*this, packet);
}

template<int N>
bool SHAKey<N>::sign(u8 *data, size_t size, u8 *signature, size_t signatureSize)
{
	return sign_(*this, data, size, signature, signatureSize);
}

template<int N>
bool SHAKey<N>::verify(u8 *data, size_t size, u8 *signature, size_t signatureSize)
{
	return verify_(*this, data, size, signature, signatureSize);
}


} // namespace
} // namespace
} // namespace
