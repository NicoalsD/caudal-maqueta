#include "caudal/ed25519.h"

#include <string.h>

#include "monocypher-ed25519.h"
#include "monocypher.h"

namespace caudal {

Ed25519Signer::Ed25519Signer(const uint8_t seed[32]) {
  // crypto_ed25519_key_pair wipes the seed buffer it receives, so work on a copy.
  uint8_t seed_copy[32];
  memcpy(seed_copy, seed, sizeof(seed_copy));
  crypto_ed25519_key_pair(secret_key_, public_key_, seed_copy);
}

Ed25519Signer::~Ed25519Signer() { crypto_wipe(secret_key_, sizeof(secret_key_)); }

void Ed25519Signer::public_key(uint8_t out[32]) const { memcpy(out, public_key_, 32); }

void Ed25519Signer::sign(const uint8_t* message, size_t len, uint8_t signature[64]) const {
  crypto_ed25519_sign(signature, secret_key_, message, len);
}

bool ed25519_verify(const uint8_t public_key[32], const uint8_t* message, size_t len,
                    const uint8_t signature[64]) {
  return crypto_ed25519_check(signature, public_key, message, len) == 0;
}

}  // namespace caudal
