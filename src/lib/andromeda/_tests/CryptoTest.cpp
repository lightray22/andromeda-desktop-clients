
#include <sodium.h>
#include "catch2/catch_test_macros.hpp"

#include "Crypto.hpp"
#include "SecureBuffer.hpp"

namespace Andromeda {
namespace { // anonymous

/*****************************************************/
TEST_CASE("GenerateSalt", "[Crypto]")
{
    REQUIRE(Crypto::SaltLength() == 16);
    REQUIRE(Crypto::GenerateSalt().size() == 16);
}

/*****************************************************/
TEST_CASE("DeriveKey", "[Crypto]")
{
    const SecureBuffer password { "mypassword123" };
    const std::string salt { "0123456789ABCDEF" }; // 16 bytes!

    REQUIRE(Crypto::DeriveKey(password, salt, 16) == 
        SecureBuffer("\x6b\xf2\xe7\xa9\x9d\x16\xa8\x18\x42\xbf\x69\x4f\xc6\xaa\xe0\x64",16));

    REQUIRE(Crypto::DeriveKey(password, salt, 24) == 
        SecureBuffer("\x0d\x34\x72\x91\xce\x9e\xca\x4b\x88\xe9\xbe\x36\xaf\x8a\x05\xae\xac\x62\x4c\x72\x4b\xd1\x7f\x5d",24));

    REQUIRE(Crypto::DeriveKey(password, salt, 32) == 
        SecureBuffer("\xa4\x9d\xd9\x7a\x61\x7a\xcd\xc0\x3b\xbd\x4f\x30\x03\xb9\xd5\xd4\x94\xc7\xff\x69\xbd\x22\x21\x84\x95\xe6\xdd\xe7\x29\xf6\xf1\x1f",32));
}

/*****************************************************/
TEST_CASE("CryptoSecret", "[Crypto]")
{
    REQUIRE(Crypto::SecretKeyLength() == 32);
    REQUIRE(Crypto::SecretNonceLength() == 24);
    REQUIRE(Crypto::SecretOutputOverhead() == 16);

    REQUIRE(Crypto::GenerateSecretKey().size() == 32);
    REQUIRE(Crypto::GenerateSecretNonce().size() == 24);

    const SecureBuffer key { "0123456789ABCDEF0123456789ABCDEF" }; // 32 bytes
    const std::string nonce { "0123456789ABCDEF01234567" }; // 24 bytes

    const SecureBuffer msg { "my super secret data..." };

    { const std::string enc { Crypto::EncryptSecret(msg, nonce, key) }; 
      REQUIRE(enc.size() == msg.size()+Crypto::SecretOutputOverhead());
      REQUIRE(enc == std::string("\x5b\x39\x4a\xd1\x17\xf0\x6e\x26\x22\x97\x4c\xb5\x79\x2d\x1a\xd4\x10\x16\x61\x3a\x37\xd9\x73\x0b\x54\xfb\x21\x4a\xae\x80\xef\xcd\x92\x18\xc0\x3f\x0c\xc8\x1e",39));
      REQUIRE(Crypto::DecryptSecret(enc, nonce, key) == msg); }

    const std::string extra { "extra auth data..." };

    const std::string enc { Crypto::EncryptSecret(msg, nonce, key, extra) };
    REQUIRE(enc.size() == msg.size()+Crypto::SecretOutputOverhead());
    REQUIRE(enc == std::string("\x5b\x39\x4a\xd1\x17\xf0\x6e\x26\x22\x97\x4c\xb5\x79\x2d\x1a\xd4\x10\x16\x61\x3a\x37\xd9\x73\xff\x34\x32\x45\x79\xb2\xd4\xbd\x0f\x81\x5a\x50\x7a\xfc\x52\x15",39));
    REQUIRE(Crypto::DecryptSecret(enc, nonce, key, extra) == msg);

    const SecureBuffer badkey { "1113456789ABCDEF0123456789ABCDEF" }; // 32 bytes
    REQUIRE_THROWS_AS(Crypto::DecryptSecret(enc, nonce, badkey, extra), Crypto::DecryptFailedException);
}

/*****************************************************/
TEST_CASE("CryptoPublic", "[Crypto]")
{
    REQUIRE(Crypto::PublicNonceLength() == 24);
    REQUIRE(Crypto::PublicOutputOverhead() == 16);
    REQUIRE(Crypto::GeneratePublicNonce().size() == 24);

    const Crypto::KeyPair keypair { Crypto::GeneratePublicKeyPair() };
    REQUIRE(keypair.pubkey.size() == 32);
    REQUIRE(keypair.privkey.size() == 32);

    const std::string pub1("\x3d\xc6\x76\x26\x8f\xc3\x6f\x9f\xe4\x5b\x06\x51\x86\x39\x0a\xdb\xcd\x88\xfa\xdc\xb8\xf9\xe6\x38\x4d\xa5\xdd\x36\x2a\x80\xac\x2d",32);
    const SecureBuffer priv1("\x29\x20\x53\x10\x9b\x4f\x6f\x89\xeb\xbd\xe2\x77\x71\xdc\x4b\x40\xbd\xdd\xff\xb7\xc4\x20\x06\x4c\xd8\x6d\x1e\x80\x5f\x26\x59\xa4",32);

    const std::string pub2("\x62\xd8\x45\x95\x24\x00\x5d\xf5\xf4\xdb\x73\x44\x53\xe8\x06\x99\x26\xb5\x63\x14\x35\x54\x5a\x81\xdf\x45\x85\x2f\x03\x2d\x14\x47",32);
    const SecureBuffer priv2("\xcf\x05\xa7\x0e\x08\x8d\xb9\x2b\xf3\xa0\xee\x12\xe2\xe8\x5f\x9f\x6f\x73\xbe\xa2\xd7\x3a\xb3\xc3\xd9\x0c\x78\x91\x23\xf3\xd3\xe0",32);
    
    const SecureBuffer msg { "my super secret data..." };
    const std::string nonce { "0123456789ABCDEF01234567" }; // 24 bytes

    const std::string enc { Crypto::EncryptPublic(msg, nonce, priv1, pub2) };
    REQUIRE(enc.size() == msg.size()+Crypto::PublicOutputOverhead());
    REQUIRE(enc == std::string("\xff\x05\xee\x40\x7b\x1f\x91\xc7\x64\xe1\x12\xf9\xc5\x9a\x97\x16\x05\x5a\x62\xd0\x02\x2d\xd9\x31\xc5\x7f\x50\xfd\x3c\x11\xdf\xca\x55\x9a\xda\xba\x70\xb7\x3e",39));
    REQUIRE(Crypto::DecryptPublic(enc, nonce, priv2, pub1) == msg);

    REQUIRE_THROWS_AS(Crypto::DecryptPublic(enc, nonce, priv1, pub1), Crypto::DecryptFailedException);
}

/*****************************************************/
TEST_CASE("CryptoAuth", "[Crypto]")
{
    REQUIRE(Crypto::AuthKeyLength() == 32);
    REQUIRE(Crypto::AuthTagLength() == 32);
    REQUIRE(Crypto::GenerateAuthKey().size() == 32);

    const std::string msg { "this should be authenticated..." };
    const SecureBuffer key { "0123456789ABCDEF0123456789ABCDEF" }; // 32 bytes

    const std::string mac { Crypto::MakeAuthCode(msg, key) };
    REQUIRE(mac.size() == Crypto::AuthTagLength());
    REQUIRE(mac == std::string("\xc4\xd6\xa1\x07\x7b\x91\x05\x11\xbb\x3f\x9a\xf7\x66\xe4\x69\x72\x6d\x3e\x46\x17\x83\xf4\x8c\xf1\x56\xe8\xd9\x96\x64\x34\xdb\xc4",32));

    REQUIRE(Crypto::TryCheckAuthCode(mac, msg, key) == true);
    Crypto::CheckAuthCode(mac, msg, key); // no throw

    const std::string badmac("\xab\xc6\xa1\x07\x7b\x91\x05\x11\xbb\x3f\x9a\xf7\x66\xe4\x69\x72\x6d\x3e\x46\x17\x83\xf4\x8c\xf1\x56\xe8\xd9\x96\x64\x34\xdb\xc4",32);
    REQUIRE(Crypto::TryCheckAuthCode(badmac, msg, key) == false);
    REQUIRE_THROWS_AS(Crypto::CheckAuthCode(badmac, msg, key), Crypto::DecryptFailedException);
}

} // namespace
} // namespace Andromeda
