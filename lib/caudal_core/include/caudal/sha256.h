// SHA-256 wrapper over the vendored public-domain implementation (sha256_vendor.h).
// Used only for public request bodies (SHA256_CUERPO in protocol section 2.2).
#ifndef CAUDAL_SHA256_H
#define CAUDAL_SHA256_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

// Digest of `len` bytes. `digest` needs 32 bytes.
void sha256(const uint8_t* data, size_t len, uint8_t* digest);

// Digest as 64 lowercase hex digits. `hex_out` needs 65 bytes (NUL included).
void sha256_hex(const uint8_t* data, size_t len, char* hex_out);

}  // namespace caudal

#endif  // CAUDAL_SHA256_H
