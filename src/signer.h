// Signing adapter (pattern P06 Adapter): wraps Monocypher Ed25519 and builds the four
// headers of every device request (protocol, section 2).
#ifndef CAUDAL_SIGNER_H
#define CAUDAL_SIGNER_H

#include <stddef.h>
#include <stdint.h>

#include "caudal/ed25519.h"

struct SignedHeaders {
  char timestamp[21];   // X-Timestamp: decimal seconds
  char nonce[33];       // X-Nonce: 32 hex digits, random
  char signature[88];   // X-Signature: base64url, 86 characters
};

class DeviceSigner {
 public:
  ~DeviceSigner();

  // Copies the device id and derives the key pair from the 32-byte seed.
  void begin(const char* device_id, const uint8_t seed[32]);

  const char* device_id() const { return device_id_; }

  // Fills the headers for one request. `body` is the exact byte sequence that will be sent.
  // Returns false if the clock has not synced yet.
  bool sign(const char* method, const char* path, const uint8_t* body, size_t len,
            SignedHeaders* out) const;

 private:
  char device_id_[37] = {0};
  caudal::Ed25519Signer* key_ = nullptr;
};

#endif  // CAUDAL_SIGNER_H
