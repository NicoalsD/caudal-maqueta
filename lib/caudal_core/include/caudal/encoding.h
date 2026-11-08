// Hex, base64url and UUID text helpers (protocol sections 1.2, 2.1 and 4.1).
// Pure C++: no Arduino dependencies.
#ifndef CAUDAL_ENCODING_H
#define CAUDAL_ENCODING_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

// Writes `n` bytes as lowercase hex followed by NUL. `out` needs 2n+1 bytes.
void to_hex(const uint8_t* in, size_t n, char* out);

// Decodes exactly `n` bytes from `2n` hex digits (upper or lower case).
bool from_hex(const char* hex, size_t hex_len, uint8_t* out, size_t n);

// Base64url without '=' padding, as the X-Signature header requires.
// `out` needs 4*ceil(n/3)+1 bytes (87 for a 64-byte signature minus padding, NUL included).
// Returns the number of characters written, without the NUL.
size_t base64url_encode(const uint8_t* in, size_t n, char* out);

// Writes an RFC 4122 version 4 UUID from 16 random bytes. `out` needs 37 bytes.
void format_uuid_v4(const uint8_t random16[16], char* out);

}  // namespace caudal

#endif  // CAUDAL_ENCODING_H
