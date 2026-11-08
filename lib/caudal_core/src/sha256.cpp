#include "caudal/sha256.h"

#include "caudal/encoding.h"
#include "caudal/sha256_vendor.h"

namespace caudal {

void sha256(const uint8_t* data, size_t len, uint8_t* digest) {
  SHA256_CTX ctx;
  sha256_init(&ctx);
  if (len > 0) sha256_update(&ctx, data, len);
  sha256_final(&ctx, digest);
}

void sha256_hex(const uint8_t* data, size_t len, char* hex_out) {
  uint8_t digest[SHA256_BLOCK_SIZE];
  sha256(data, len, digest);
  to_hex(digest, SHA256_BLOCK_SIZE, hex_out);
}

}  // namespace caudal
