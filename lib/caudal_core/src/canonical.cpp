#include "caudal/canonical.h"

#include <string.h>

#include "caudal/sha256.h"

namespace caudal {

namespace {

// Appends `text` plus an optional separator. Returns false when it does not fit.
bool append(char* out, size_t cap, size_t* pos, const char* text) {
  const size_t n = strlen(text);
  if (*pos + n + 1 > cap) return false;
  memcpy(out + *pos, text, n);
  *pos += n;
  return true;
}

bool append_int64(char* out, size_t cap, size_t* pos, int64_t value) {
  char digits[24];
  int len = 0;
  uint64_t magnitude = value < 0 ? static_cast<uint64_t>(-(value + 1)) + 1 : value;
  do {
    digits[len++] = static_cast<char>('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude > 0);
  char text[25];
  int n = 0;
  if (value < 0) text[n++] = '-';
  while (len > 0) text[n++] = digits[--len];
  text[n] = '\0';
  return append(out, cap, pos, text);
}

}  // namespace

void body_sha256_hex(const uint8_t* body, size_t len, char* out) { sha256_hex(body, len, out); }

size_t build_canonical_string(const char* method, const char* path, int64_t timestamp,
                              const char* nonce_hex, const char* body_sha256_hex, char* out,
                              size_t cap) {
  size_t pos = 0;
  if (!append(out, cap, &pos, method) || !append(out, cap, &pos, "\n") ||
      !append(out, cap, &pos, path) || !append(out, cap, &pos, "\n") ||
      !append_int64(out, cap, &pos, timestamp) || !append(out, cap, &pos, "\n") ||
      !append(out, cap, &pos, nonce_hex) || !append(out, cap, &pos, "\n") ||
      !append(out, cap, &pos, body_sha256_hex)) {
    return 0;
  }
  out[pos] = '\0';
  return pos;
}

}  // namespace caudal
