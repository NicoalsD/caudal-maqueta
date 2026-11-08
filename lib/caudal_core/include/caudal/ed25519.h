// Ed25519 signatures over Monocypher's crypto_ed25519_* (RFC 8032), not the
// BLAKE2b-based crypto_eddsa_* family, so signatures match the backend's Ed25519.
#ifndef CAUDAL_ED25519_H
#define CAUDAL_ED25519_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

class Ed25519Signer {
 public:
  // Derives the key pair from the 32-byte seed. The seed is the private key of
  // the device (protocol section 1.2); it never leaves the device.
  explicit Ed25519Signer(const uint8_t seed[32]);
  ~Ed25519Signer();

  Ed25519Signer(const Ed25519Signer&) = delete;
  Ed25519Signer& operator=(const Ed25519Signer&) = delete;

  void public_key(uint8_t out[32]) const;
  void sign(const uint8_t* message, size_t len, uint8_t signature[64]) const;

 private:
  uint8_t secret_key_[64];
  uint8_t public_key_[32];
};

// True when `signature` is a valid Ed25519 signature of `message` under `public_key`.
bool ed25519_verify(const uint8_t public_key[32], const uint8_t* message, size_t len,
                    const uint8_t signature[64]);

}  // namespace caudal

#endif  // CAUDAL_ED25519_H
