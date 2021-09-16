#include "Crypto.h"

#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/engine.h>

namespace timprepscius {
namespace mrudp {
namespace imp {

struct Engine
{
	ENGINE *native;
	
	Engine()
	{
		native = nullptr;
	}
	
	static Engine *shared;
} ;

#define SSL_FAIL(x) (x) <= 0

Engine *Engine::shared = new Engine();

// ------------------------------------------------------------

struct SecureRandom::I {

	bool generate(u8 *data, size_t size)
	{
		if (SSL_FAIL(RAND_bytes(data, (int)size)))
			return false;
			
		return true;
	}
} ;

SecureRandom::SecureRandom()
{
	i = new I;
}

SecureRandom::~SecureRandom()
{
	delete i;
}

bool SecureRandom::generate(u8 *data, size_t size)
{
	return i->generate(data, size);
}

// ------------------------------------------------------------

using EVP_PKEY_CTX_ptr = std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;

template<int N>
struct RSAKey<N>::I {
	ENGINE *engine = nullptr;
	EVP_PKEY *pkey = nullptr;
	Vector<u8> bytes;
} ;

bool construct_(RSAKeyDefault &public_, Vector<u8> &&bytes)
{
	public_.i = new RSAKeyDefault::I;
	
	auto &i = *public_.i;
	i.engine = Engine::shared->native;
	i.bytes = std::move(bytes);
	
	const u8 *bytes_r = public_.i->bytes.data();
	auto len = public_.i->bytes.size();
	if (!d2i_PublicKey(EVP_PKEY_RSA, &i.pkey, &bytes_r, len))
		return false;
	
	if (!i.pkey)
		return false;
		
	return true;
}

//template<>
bool construct_(SecureRandom &random, RSAKeyDefault &private_, RSAKeyDefault &public_)
{
	// generate the private key
	{
		private_.i = new RSAKeyDefault::I;
		
		auto &i = *private_.i;
		i.engine = Engine::shared->native;
		
		EVP_PKEY_CTX_ptr ctx_ = EVP_PKEY_CTX_ptr(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, i.engine), ::EVP_PKEY_CTX_free);
		auto ctx = ctx_.get();
		
		if (!ctx)
			return false;
			
		if (SSL_FAIL(EVP_PKEY_keygen_init(ctx)))
			return false;
			
		if (SSL_FAIL(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, private_.BitSize)))
			return false;
		
		if (SSL_FAIL(EVP_PKEY_keygen(ctx, &i.pkey)))
			return false;

		xDebugLine();
	}
	
	// generate the public key
	{
		u8 *bytes = nullptr;
		auto len = i2d_PublicKey(private_.i->pkey, &bytes);
		if (SSL_FAIL(len))
			return false;

		Vector<u8> v(bytes, bytes+len);
		if (!construct_(public_, std::move(v)))
			return false;
	}
	
	return true;
}

//template<>
void destruct_(RSAKeyDefault &key)
{
	if (key.i)
	{
		EVP_PKEY_free(key.i->pkey);
		delete key.i;
	}
}

// ------------------------------------------

//template<>
bool encrypt_(RSAKeyDefault &rsa, const AESKey<DefaultAESKeySize> &in, Vector<u8> &out, SecureRandom &random)
{
	// https://www.openssl.org/docs/man1.1.0/man3/EVP_PKEY_encrypt.html

	auto &i = *rsa.i;
	
	EVP_PKEY_CTX_ptr ctx_ = EVP_PKEY_CTX_ptr(EVP_PKEY_CTX_new(i.pkey, i.engine), ::EVP_PKEY_CTX_free);
	auto ctx = ctx_.get();
	
	if (!ctx)
		return false;
		
	if (SSL_FAIL(EVP_PKEY_encrypt_init(ctx)))
		return false;
		
	if (SSL_FAIL(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING)))
		return false;
		
	size_t size = 0;
	if (SSL_FAIL(EVP_PKEY_encrypt(ctx, NULL, &size, in.data, in.ByteSize)))
		return false;
	
	out.resize(size);
	if (SSL_FAIL(EVP_PKEY_encrypt(ctx, out.data(), &size, in.data, in.ByteSize)))
		return false;
		
	out.resize(size);
	
	return true;
}

//template<>
bool decrypt_(RSAKeyDefault &rsa, const Vector<u8> &in, AESKey<DefaultAESKeySize> &out)
{
	auto &i = *rsa.i;

	EVP_PKEY_CTX_ptr ctx_ = EVP_PKEY_CTX_ptr(EVP_PKEY_CTX_new(i.pkey, i.engine), ::EVP_PKEY_CTX_free);
	auto ctx = ctx_.get();

	if (EVP_PKEY_decrypt_init(ctx) <= 0)
		return false;
		
	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
		return false;

	size_t size = 0;
	if (SSL_FAIL(EVP_PKEY_decrypt(ctx, NULL, &size, in.data(), in.size())))
		return false;

	// use the stack
	u8 *temporary = (u8 *)alloca(size);
	if (!temporary)
		return false;

	if (SSL_FAIL(EVP_PKEY_decrypt(ctx, temporary, &size, in.data(), in.size())))
		return false;

	// take it off of the stack
	if (size != out.ByteSize)
		return false;
		
	memcpy(out.data, temporary, size);
	return true;
}

// ------------------------------------------

using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)>;

//template<>
bool construct_(SecureRandom &random, AESKeyDefault &key)
{
	return random.generate(key.data, key.ByteSize);
}

template<int N>
bool construct_(SecureRandom &random, AESIV<N> &iv)
{
	return random.generate(iv.data, iv.ByteSize);
}

//template<>
bool encrypt_(RSAKeyDefault &rsa, Packet &packet, size_t maxPadTo, SecureRandom &random)
{
	// generate SHAKeys
	AESKeyDefault aesKey;
	if (!construct_(random, aesKey))
		return false;
	
	Vector<u8> encryptedAESKey;
	if (!rsa.encrypt(aesKey, encryptedAESKey, random))
		return false;

	if (!aesKey.encrypt(packet, maxPadTo - pushSize(encryptedAESKey), random))
		return false;
		
	if (!pushData(packet, encryptedAESKey))
		return false;
		
	packet.header.type = ENCRYPTED_VIA_PUBLIC_KEY;
	
	return true;
}

//template<>
bool decrypt_(RSAKeyDefault &rsa, Packet &packet)
{
	Vector<u8> encryptedAESKey;
	if (!popData(packet, encryptedAESKey))
		return false;
		
	AESKeyDefault aesKey;
	if (!rsa.decrypt(encryptedAESKey, aesKey))
		return false;
		
	packet.header.type = ENCRYPTED_VIA_AES;
	return aesKey.decrypt(packet);
}

uint16_t calculatePadding(Packet &packet, size_t maxPadTo_, SecureRandom &random)
{
	uint16_t padding = 0;

	// the actual pad to needs to take into account the padding variable
	auto maxPadTo = maxPadTo_ - sizeof(padding);

	random.generate((u8 *)&padding, sizeof(padding));
	
	// find the clipped padding addendum
	padding = padding % (maxPadTo - packet.dataSize);
	
	return padding;
}

bool pushPadding(Packet &packet, size_t padding, SecureRandom &random)
{
	return true;
	
	debug_assert(packet.dataSize + padding < MAX_PACKET_POST_CRYPTO_SIZE);
	if (packet.dataSize + padding >= MAX_PACKET_POST_CRYPTO_SIZE)
	{
		xDebugLine();
		return false;
	}
	
	// let's set that data to zero, just in case there is some
	// attack
	memset(packet.data + packet.dataSize, (int)'_', padding);
	packet.dataSize += padding;
	
	// push the actual number
	if (!pushData(packet, (u16)padding))
		return false;
		
	return true;
}

bool popPadding(Packet &packet)
{
	return true;
	
	uint16_t padding;
	if (!popData(packet, padding))
		return false;

	if (packet.dataSize < padding)
		return false;
		
	packet.dataSize -= padding;
	
	return true;
}

//template<>
bool encrypt_(AESKeyDefault &key, Packet &packet, size_t maxPadTo, SecureRandom &random)
{
	// generate the public IV
	AESIVDefault iv;
	if (!construct_(random, iv))
		return false;

	// move the header id into the packet data
	if (!pushData(packet, packet.header.id))
		return false;
		
	// move the packet type into the packet data
	if (!pushData(packet, packet.header.type))
		return false;
		
	// erase information
	packet.header.id = 0;
	packet.header.type = ENCRYPTED_VIA_AES;

	// generate the random number for padding
	auto padding = calculatePadding(packet, maxPadTo, random);
	pushPadding(packet, padding, random);

	Packet::Data data;
	memcpy(data, packet.data, packet.dataSize);

	auto doCrypto = true;
	if (doCrypto)
	{
		auto iv_ = iv;

		EVP_CIPHER_CTX_ptr ctx_ = EVP_CIPHER_CTX_ptr(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
		auto ctx = ctx_.get();
		
		if (!ctx)
			return false;

		// I believe, that because BitSize is a constexpr, one of the below statements will
		// optimized out
		if (key.BitSize == 256)
		{
			if(SSL_FAIL(EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data, iv_.data)))
				return false;
		}
		else
		if (key.BitSize == 128)
		{
			if(SSL_FAIL(EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key.data, iv_.data)))
				return false;
		}
		else
		{
			return false;
		}

		int size = 0;
		
		if(SSL_FAIL(EVP_EncryptUpdate(ctx, (u8 *)packet.data, &size, (u8 *)data, packet.dataSize)))
			return false;

		packet.dataSize = size;

		if(SSL_FAIL(EVP_EncryptFinal(ctx, (u8 *)packet.data + packet.dataSize, &size)))
			return false;
			
		packet.dataSize += size;
	}

	if (!pushData(packet, iv))
		return false;
	
	return true;
}

//template<>
bool decrypt_(AESKeyDefault &key, Packet &packet)
{
	AESIVDefault iv;
	if (!popData(packet, iv))
		return false;

	Packet::Data data;
	memcpy(data, packet.data, packet.dataSize);

	auto doCrypto = true;
	if (doCrypto)
	{
		auto iv_ = iv;

		EVP_CIPHER_CTX_ptr ctx_ = EVP_CIPHER_CTX_ptr(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
		auto ctx = ctx_.get();
		
		if (!ctx)
			return false;

		// I believe, that because BitSize is a constexpr, one of the below statements will
		// optimized out
		if (key.BitSize == 256)
		{
			if(SSL_FAIL(EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data, iv_.data)))
				return false;
		}
		else
		if (key.BitSize == 128)
		{
			if(SSL_FAIL(EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key.data, iv_.data)))
				return false;
		}
		else
		{
			return false;
		}

		int size;
		if(SSL_FAIL(EVP_DecryptUpdate(ctx, (u8 *)packet.data, &size, (u8 *)data, packet.dataSize)))
			return false;

		packet.dataSize = size;

		if(SSL_FAIL(EVP_DecryptFinal(ctx, (u8 *)packet.data + packet.dataSize, &size)))
			return false;

		packet.dataSize += size;
	}

	if (!popPadding(packet))
		return false;

	if (!popData(packet, packet.header.type))
		return false;
		
	// linux doesn't allow the u16 & in a packed structure.
	// so we can't say, popData(packet, packet.header.id)
	if (!popData(packet, (u8 *)&packet.header.id, sizeof(packet.header.id)))
		return false;
	
	return true;
}

// -----------------

using EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

//template<>
bool construct_(SecureRandom &random, SHAKeyDefault &key)
{
	return random.generate(key.data, key.ByteSize);
}

bool sign_(SHAKeyDefault &key, u8 *data, size_t size, u8 *signature, size_t signatureSize)
{
	auto doCrypto = true;
	if (doCrypto)
	{
		EVP_MD_CTX_ptr ctx_ = EVP_MD_CTX_ptr(EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
		auto ctx = ctx_.get();
		
		if (!ctx)
			return false;

		// I believe, that because BitSize is a constexpr, one of the below statements will
		// optimized out
		if (key.BitSize == 256)
		{
			if(SSL_FAIL(EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)))
				return false;
		}
		else
		if (key.BitSize == 128)
		{
			if(SSL_FAIL(EVP_DigestInit_ex(ctx, EVP_sha1(), NULL)))
				return false;
		}
		else
		{
			return false;
		}

		if(SSL_FAIL(EVP_DigestUpdate(ctx, data, size)))
			return false;

		memset(signature, 0, signatureSize);
		unsigned int signatureSize_ = signatureSize;
		if(SSL_FAIL(EVP_DigestFinal_ex(ctx, signature, &signatureSize_)))
			return false;
			
		if (signatureSize_ != signatureSize)
			return false;
	}

	return true;
}

//template<>
bool verify_(SHAKeyDefault &key, u8 *data, size_t size, u8 *compare, size_t compareSize)
{
	auto doCrypto = true;
	if (doCrypto)
	{
		EVP_MD_CTX_ptr ctx_ = EVP_MD_CTX_ptr(EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
		auto ctx = ctx_.get();
		
		if (!ctx)
			return false;

		// I believe, that because BitSize is a constexpr, one of the below statements will
		// optimized out
		if (key.BitSize == 256)
		{
			if(SSL_FAIL(EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)))
				return false;
		}
		else
		if (key.BitSize == 128)
		{
			if(SSL_FAIL(EVP_DigestInit_ex(ctx, EVP_sha1(), NULL)))
				return false;
		}
		else
		{
			return false;
		}

		if(SSL_FAIL(EVP_DigestUpdate(ctx, data, size)))
			return false;

		u8 *signature = (u8 *)alloca(key.ByteSize);
		unsigned int signatureSize_ = key.ByteSize;
		memset(signature, 0, signatureSize_);
		
		if(SSL_FAIL(EVP_DigestFinal_ex(ctx, signature, &signatureSize_)))
			return false;
			
		if (signatureSize_ != compareSize)
			return false;
			
		if (memcmp(signature, compare, compareSize) != 0)
			return false;
	}

	return true;
}

// -----------------

} // namespace

template<>
bool pushData(Packet &packet, const RSAPublicKey &publicKey)
{
	return pushData(packet, publicKey.i->bytes);
}

template<>
bool popData(Packet &packet, RSAPublicKey &publicKey)
{
	Vector<u8> bytes;
	if (!popData(packet, bytes))
		return false;
		
	return construct_(publicKey, std::move(bytes));
}

RSAPrivatePublicKeyPair generateRSAPrivatePublicKeyPair(SecureRandom &random)
{
	auto private_ = strong<RSAPrivateKey>();
	auto public_ = strong<RSAPublicKey>();
	
	imp::construct_(random, *private_, *public_);
	
	return RSAPrivatePublicKeyPair { private_ , public_ };
}

StrongPtr<AESKey> generateAESKey (SecureRandom &random)
{
	auto aesKey = strong<AESKey>();
	imp::construct_(random, *aesKey);

	return aesKey;
}

StrongPtr<SHAKey> generateSHAKey (SecureRandom &random)
{
	auto shaKey = strong<SHAKey>();
	imp::construct_(random, *shaKey);

	return shaKey;
}

} // namespace
} // namespace
