
#include "sodium.h"

#include "Crypto.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace { // anonymous
Debug sDebug("Crypto",nullptr); // NOLINT(cert-err58-cpp)
} // anonymous namespace

/*****************************************************/
void SodiumInit() // @throws SodiumFailedException
{
    const int initc { sodium_init() };
    if (initc < 0)
    {
        SDBG_ERROR("... sodium_init() failed!");
        throw Crypto::SodiumFailedException(initc);
    }
}

/*****************************************************/
std::string Crypto::GenerateRandom(size_t len)
{
    SodiumInit();

    std::string ret; ret.resize(len);
    randombytes_buf(ret.data(), len);
    return ret;
}

/*****************************************************/
size_t Crypto::SaltLength()
{
    return crypto_pwhash_argon2id_SALTBYTES;
}

/*****************************************************/
std::string Crypto::GenerateSalt()
{
    return GenerateRandom(SaltLength());
}

/*****************************************************/
std::string Crypto::DeriveKey(const std::string& password, const std::string& salt, size_t bytes)
{
    if (salt.size() != SaltLength())
        throw ArgumentException("salt was "+std::to_string(salt.size())+" bytes");

    SodiumInit();

    std::string key; key.resize(bytes);
    const int err = crypto_pwhash( 
        reinterpret_cast<unsigned char*>(key.data()), key.size(),
        password.data(), password.size(), 
        reinterpret_cast<const unsigned char*>(salt.data()),
        crypto_pwhash_argon2id_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_argon2id_MEMLIMIT_INTERACTIVE,
        crypto_pwhash_argon2id_ALG_ARGON2ID13
    );
    
    if (err)
    {
        SDBG_ERROR("... crypto_pwhash returned " << err);
        throw SodiumFailedException(err);
    }
    return key;
}

/*****************************************************/
size_t Crypto::SecretKeyLength()
{
    return crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
}

/*****************************************************/
size_t Crypto::SecretNonceLength()
{
    return crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
}

/*****************************************************/
size_t Crypto::SecretOutputOverhead()
{
    return crypto_aead_xchacha20poly1305_ietf_ABYTES;
}

/*****************************************************/
std::string Crypto::GenerateSecretKey()
{
    return GenerateRandom(SecretKeyLength());
}

/*****************************************************/
std::string Crypto::GenerateSecretNonce()
{
    return GenerateRandom(SecretNonceLength());
}

/*****************************************************/
std::string Crypto::EncryptSecret(const std::string& msg, const std::string& nonce, const std::string& key, const std::string& extra)
{
    if (nonce.size() != SecretNonceLength())
        throw ArgumentException("nonce was "+std::to_string(nonce.size())+" bytes");
    if (key.size() != SecretKeyLength())
        throw ArgumentException("key was "+std::to_string(key.size())+" bytes");

    SodiumInit();

    std::string enc; enc.resize(msg.size()+SecretOutputOverhead());

    unsigned long long clen { 0 }; // NOLINT(google-runtime-int)
    const int err = crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char*>(enc.data()), &clen,
        reinterpret_cast<const unsigned char*>(msg.data()), msg.size(),
        reinterpret_cast<const unsigned char*>(extra.data()), extra.size(),
        nullptr, // nsec is unused
        reinterpret_cast<const unsigned char*>(nonce.data()),
        reinterpret_cast<const unsigned char*>(key.data())
    );

    if (err)
    {
        SDBG_ERROR("... crypto_encrypt returned " << err);
        throw SodiumFailedException(err);
    }

    if (!clen || clen > enc.size())
    {
        SDBG_ERROR("... bad clen:" << clen);
        throw SodiumFailedException(0);
    }

    enc.resize(static_cast<size_t>(clen)); // maybe smaller
    return enc;
}

/*****************************************************/
std::string Crypto::DecryptSecret(const std::string& enc, const std::string& nonce, const std::string& key, const std::string& extra)
{
    if (nonce.size() != SecretNonceLength())
        throw ArgumentException("nonce was "+std::to_string(nonce.size())+" bytes");
    if (key.size() != SecretKeyLength())
        throw ArgumentException("key was "+std::to_string(key.size())+" bytes");

    SodiumInit();

    std::string msg; msg.resize(enc.size());

    unsigned long long mlen { 0 }; // NOLINT(google-runtime-int)
    const int err = crypto_aead_xchacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char*>(msg.data()), &mlen,
        nullptr, // nsec is unused
        reinterpret_cast<const unsigned char*>(enc.data()), enc.size(),
        reinterpret_cast<const unsigned char*>(extra.data()), extra.size(),
        reinterpret_cast<const unsigned char*>(nonce.data()),
        reinterpret_cast<const unsigned char*>(key.data())
    );

    if (err)
    {
        // not an ERROR, this can happen with just a bad key...
        SDBG_INFO("... crypto_decrypt returned " << err);
        throw DecryptFailedException(err);
    }

    if (!mlen || mlen > msg.size())
    {
        SDBG_ERROR("... bad mlen:" << mlen);
        throw SodiumFailedException(0);
    }

    msg.resize(static_cast<size_t>(mlen)); // maybe smaller
    return msg;
}

/*****************************************************/
size_t Crypto::PublicNonceLength()
{
    return crypto_box_NONCEBYTES;
}

/*****************************************************/
std::string Crypto::GeneratePublicNonce()
{
    return GenerateRandom(PublicNonceLength());
}

/*****************************************************/
Crypto::KeyPair Crypto::GeneratePublicKeyPair()
{
    SodiumInit();

    KeyPair keypair;
    keypair.pubkey.resize(crypto_box_PUBLICKEYBYTES);
    keypair.privkey.resize(crypto_box_SECRETKEYBYTES);

    const int err = crypto_box_keypair(
        reinterpret_cast<unsigned char*>(keypair.pubkey.data()),
        reinterpret_cast<unsigned char*>(keypair.privkey.data())
    );

    if (err)
    {
        SDBG_ERROR("... crypto_box_keypair returned " << err);
        throw SodiumFailedException(err);
    }

    return keypair;
}

/*****************************************************/
size_t Crypto::PublicOutputOverhead()
{
    return crypto_box_MACBYTES;
}

/*****************************************************/
std::string Crypto::EncryptPublic(const std::string& msg, const std::string& nonce, const std::string& sender_private, const std::string& recipient_public)
{
    if (nonce.size() != PublicNonceLength())
        throw ArgumentException("nonce was "+std::to_string(nonce.size())+" bytes");
    if (sender_private.size() != crypto_box_SECRETKEYBYTES)
        throw ArgumentException("privkey was "+std::to_string(sender_private.size())+" bytes");
    if (recipient_public.size() != crypto_box_PUBLICKEYBYTES)
        throw ArgumentException("pubkey was "+std::to_string(recipient_public.size())+" bytes");

    SodiumInit();

    std::string enc; enc.resize(msg.size()+PublicOutputOverhead());

    const int err = crypto_box_easy(
        reinterpret_cast<unsigned char*>(enc.data()),
        reinterpret_cast<const unsigned char*>(msg.data()), msg.size(),
        reinterpret_cast<const unsigned char*>(nonce.data()),
        reinterpret_cast<const unsigned char*>(recipient_public.data()),
        reinterpret_cast<const unsigned char*>(sender_private.data())
    );

    if (err)
    {
        SDBG_ERROR("... crypto_box returned " << err);
        throw SodiumFailedException(err);
    }

    return enc;
}

/*****************************************************/
std::string Crypto::DecryptPublic(const std::string& enc, const std::string& nonce, const std::string& recipient_private, const std::string& sender_public)
{
    if (nonce.size() != PublicNonceLength())
        throw ArgumentException("nonce was "+std::to_string(nonce.size())+" bytes");
    if (recipient_private.size() != crypto_box_SECRETKEYBYTES)
        throw ArgumentException("privkey was "+std::to_string(recipient_private.size())+" bytes");
    if (sender_public.size() != crypto_box_PUBLICKEYBYTES)
        throw ArgumentException("pubkey was "+std::to_string(sender_public.size())+" bytes");

    SodiumInit();

    std::string msg; msg.resize(enc.size()-PublicOutputOverhead());

    const int err = crypto_box_open_easy(
        reinterpret_cast<unsigned char*>(msg.data()), 
        reinterpret_cast<const unsigned char*>(enc.data()), enc.size(),
        reinterpret_cast<const unsigned char*>(nonce.data()),
        reinterpret_cast<const unsigned char*>(sender_public.data()),
        reinterpret_cast<const unsigned char*>(recipient_private.data())
    );

    if (err)
    {
        // not an ERROR, this can happen with just a bad key...
        SDBG_INFO("... crypto_box_open returned " << err);
        throw DecryptFailedException(err);
    }

    return msg;
}

/*****************************************************/
size_t Crypto::AuthKeyLength()
{
    return crypto_auth_KEYBYTES;
}

/*****************************************************/
size_t Crypto::AuthTagLength()
{
    return crypto_auth_BYTES;
}

/*****************************************************/
std::string Crypto::GenerateAuthKey()
{
    return GenerateRandom(AuthKeyLength());
}

/*****************************************************/
std::string Crypto::MakeAuthCode(const std::string& msg, const std::string& key)
{
    if (key.size() != AuthKeyLength())
        throw ArgumentException("key was "+std::to_string(key.size())+" bytes");

    SodiumInit();

    std::string mac; mac.resize(AuthTagLength());

    const int err = crypto_auth(
        reinterpret_cast<unsigned char*>(mac.data()),
        reinterpret_cast<const unsigned char*>(msg.data()), msg.size(),
        reinterpret_cast<const unsigned char*>(key.data())
    );

    if (err)
    {
        SDBG_ERROR("... crypto_auth returned " << err);
        throw SodiumFailedException(err);
    }

    return mac;
}

/*****************************************************/
bool Crypto::TryCheckAuthCode(const std::string& mac, const std::string& msg, const std::string& key)
{
    if (key.size() != AuthKeyLength())
        throw ArgumentException("key was "+std::to_string(key.size())+" bytes");
    if (mac.size() != AuthTagLength())
        throw ArgumentException("mac was "+std::to_string(mac.size())+" bytes");

    SodiumInit();

    const int err = crypto_auth_verify(
        reinterpret_cast<const unsigned char*>(mac.data()),
        reinterpret_cast<const unsigned char*>(msg.data()), msg.size(),
        reinterpret_cast<const unsigned char*>(key.data())
    );

    if (err)
    {
        // not an ERROR, this can happen with just a bad key...
        SDBG_INFO("... crypto_auth_verify returned " << err);
        return false; // no throw
    }
    return true;
}

/*****************************************************/
void Crypto::CheckAuthCode(const std::string& mac, const std::string& msg, const std::string& key)
{
    if (!TryCheckAuthCode(mac, msg, key))
        throw DecryptFailedException(0);
}

} // namespace Andromeda
