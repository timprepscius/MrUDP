#pragma once

#include "Crypto.h"

namespace timprepscius {
namespace mrudp {
namespace crypto {
namespace imp {

// --------------------------

template<typename N>
void encrypt(RSAKey<N> &key, Packet &packet);

template<typename N>
void decrypt(RSAKey<N> &key, Packet &packet);

template<typename N>
void encrypt(SHAKey<N> &key, Packet &packet);

template<typename N>
void decrypt(SHAKey<N> &key, Packet &packet);

// --------------------------

template<>
void encrypt(RSAKey<DefaultRSAKeySize> &key, Packet &packet);

template<>
void decrypt(RSAKey<DefaultRSAKeySize> &key, Packet &packet);

template<>
void encrypt(SHAKey<256> &key, Packet &packet);

template<>
void decrypt(SHAKey<256> &key, Packet &packet);

// --------------------------

template<typename N>
void RSAKey<N>::decrypt(Packet &packet)
{
	return decrypt(*this, packet);
}

template<typename N>
void RSAKey<N>::encrypt(Packet &packet)
{
	return encrypt(*this, packet);
}

template<typename N>
void SHAKey<N>::decrypt(Packet &packet)
{
	return decrypt(*this, packet);
}

template<typename N>
void SHAKey<N>::encrypt(Packet &packet)
{
	return encrypt(*this, packet);
}

} // namespace
} // namespace
} // namespace
} // namespace
