#ifndef LIBA2_CRYPTO_H_
#define LIBA2_CRYPTO_H_

#include <string>
#include <utility>
#include "andromeda/BaseException.hpp"
#include "andromeda/SecureBuffer.hpp"

namespace Andromeda {

/** 
 * libsodium wrapper class for crypto 
 * ported from the server's PHP implementation
 */
class Crypto
{
public:

    Crypto() = delete; // static only

    /** Base Exception indicating a failure with a crypto operation */
    class Exception : public BaseException { public:
        explicit Exception(const std::string& msg) : BaseException("Crypto error: "+msg) {}; };

    /** Exception indicating a function was given an invalid argument */
    class ArgumentException : public Exception { public:
        explicit ArgumentException(const std::string& msg) : Exception("Invalid argument: "+msg) {}; };

    /** Exception indicating that a sodium operation failed */
    class SodiumFailedException : public Exception { public:
        explicit SodiumFailedException(int code) : Exception("Sodium failed: "+std::to_string(code)) {}; };

    /** Exception indicating that a sodium operation failed to decrypt */
    class DecryptFailedException : public Exception { public:
        explicit DecryptFailedException(int code) : Exception("Decryption failed: "+std::to_string(code)) {}; };

    /** 
     * Generates a random string suitable for cryptography
     * @throws SodiumFailedException
     */
    static std::string GenerateRandom(size_t len);
    /** 
     * Generates a random SecureBuffer suitable for cryptography
     * @throws SodiumFailedException
     */
    static SecureBuffer GenerateSecRandom(size_t len);

    /** Returns the length of a generated salt */
    static size_t SaltLength();

    /** 
     * Generates a salt for use with DeriveKey
     * @throws SodiumFailedException 
     */
    static std::string GenerateSalt();

    /** 
     * Generates an encryption key from a password
     * @param password the password to derive the key from
     * @param salt a generated salt to use (see GenerateSalt)
     * @param bytes the number of bytes required to output
     * @return the derived binary key string
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
    */
    static SecureBuffer DeriveKey(const SecureBuffer& password, const std::string& salt, size_t bytes = SecretKeyLength());
    // TODO maybe add support for "sensitive" hash, right now only have "interactive" (later, for signing in, etc.)

    /** Returns the length of a key for use with secret crypto */
    static size_t SecretKeyLength();

    /** Returns the length of a nonce for use with secret crypto */
    static size_t SecretNonceLength();

    /** 
     * Returns the size overhead of an encrypted string over a plaintext one
     * The size overhead exists because the crypto is authenticated
     */
    static size_t SecretOutputOverhead();

    /** 
     * Generates a crypto key for use with secret crypto 
     * @throws SodiumFailedException
     */
    static SecureBuffer GenerateSecretKey();

    /** 
     * Generates a crypto nonce for use with secret crypto
     * @throws SodiumFailedException
     */
    static std::string GenerateSecretNonce();

    /**
     * Encrypts the given data
     * @param msg the plaintext to encrypt
     * @param nonce the crypto nonce to use
     * @param key the crypto key to use
     * @param extra extra data to include in authentication (must be provided at decrypt)
     * @return std::string an encrypted and authenticated ciphertext
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
     */
    static std::string EncryptSecret(const SecureBuffer& msg, const std::string& nonce, const SecureBuffer& key, const std::string& extra = "");

    /**
     * Decrypts the given data
     * @param enc the ciphertext to decrypt
     * @param nonce the nonce that was used to encrypt
     * @param key the key that was used to encrypt
     * @param extra the extra auth data used to encrypt
     * @return SecureBuffer the decrypted and authenticated plaintext
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
     * @throws DecryptFailedException
     */
    static SecureBuffer DecryptSecret(const std::string& enc, const std::string& nonce, const SecureBuffer& key, const std::string& extra = "");

    /** Returns the length of a nonce for use with public crypto */
    static size_t PublicNonceLength();

    /** 
     * Generates a crypto nonce for use with public crypto
     * @throws SodiumFailedException
     */
    static std::string GeneratePublicNonce();

    /** A public/private keypair */
    struct KeyPair
    {
        std::string pubkey;
        SecureBuffer privkey;
    };

    /** 
     * Generates a public/private keypair
     * @throws SodiumFailedException
     */
    static KeyPair GeneratePublicKeyPair();

    /** Returns the size overhead of an encrypted string over a plaintext one */
    static size_t PublicOutputOverhead();

    /**
     * Encrypts and signs data from a sender to a recipient
     * @param msg the plaintext to encrypt
     * @param nonce the nonce to encrypt with
     * @param sender_private the sender's private key
     * @param recipient_public the recipient's public key
     * @return std::string the encrypted and signed ciphertext
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
     */
    static std::string EncryptPublic(const SecureBuffer& msg, const std::string& nonce, const SecureBuffer& sender_private, const std::string& recipient_public);

    /**
     * Decrypts and verifies data from a sender to a recipient
     * @param enc the ciphertext to decrypt
     * @param nonce the nonce that was used to encrypt
     * @param recipient_private the recipient's private key
     * @param sender_public the sender's public key
     * @return SecureBuffer the decrypted and verified plaintext
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
     * @throws DecryptFailedException
     */
    static SecureBuffer DecryptPublic(const std::string& enc, const std::string& nonce, const SecureBuffer& recipient_private, const std::string& sender_public);

    /** Returns the length of a key for use with auth crypto */
    static size_t AuthKeyLength();

    /** Returns the length of the authentication tag generated */
    static size_t AuthTagLength();

    /** 
     * Generates a crypto key for use with auth crypto
     * @throws SodiumFailedException
     */
    static SecureBuffer GenerateAuthKey();

    /**
     * Creates an authentication code (MAC) from a message and key
     * @param msg the message to create the MAC for
     * @param key the secret key to use for creating the MAC
     * @return std::string the message authentication code (MAC)
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
     */
    static std::string MakeAuthCode(const std::string& msg, const SecureBuffer& key);

    /**
     * Tries to authenticate a message using a secret key
     * @param mac the message authentication code
     * @param msg the message to verify
     * @param key the key used in creating the MAC
     * @return bool true if authentication succeeds
     * @throws InvalidArgumentException
     * @throws SodiumFailedException
     */
    static bool TryCheckAuthCode(const std::string& mac, const std::string& msg, const SecureBuffer& key);

    /**
     * Same as TryCheckAuthCode() but throws an exception on decrypt failure
     * @see TryCheckAuthCode()
     * @throws DecryptFailedException
     */
    static void CheckAuthCode(const std::string& mac, const std::string& msg, const SecureBuffer& key);

private:

    friend struct SecureMemory; // SodiumInit

    /**
     * Calls sodium_init() to initialize
     * @throws SodiumFailedException
     */
    static void SodiumInit();
};

} // namespace Andromeda

#endif // LIBA2_CRYPTO_H_
