// Canonical string of a signed device request (protocol section 2.2).
// Pure C++: the firmware and the tests build the exact same bytes.
#ifndef CAUDAL_CANONICAL_H
#define CAUDAL_CANONICAL_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

// SHA-256 of the body bytes as 64 lowercase hex digits. `out` needs 65 bytes.
// An empty body yields e3b0c442...b855 (the hash of the empty string).
void body_sha256_hex(const uint8_t* body, size_t len, char* out);

// Builds "METHOD\nPATH\nTIMESTAMP\nNONCE\nSHA256_BODY" without a final LF.
// `nonce_hex` is 32 lowercase hex digits. Returns the length written, or 0 if
// `cap` is too small (the output is then not NUL-terminated).
size_t build_canonical_string(const char* method, const char* path, int64_t timestamp,
                              const char* nonce_hex, const char* body_sha256_hex, char* out,
                              size_t cap);

}  // namespace caudal

#endif  // CAUDAL_CANONICAL_H
