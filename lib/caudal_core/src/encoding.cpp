#include "caudal/encoding.h"

namespace caudal {

namespace {

const char kHexDigits[] = "0123456789abcdef";
const char kBase64Url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

}  // namespace

void to_hex(const uint8_t* in, size_t n, char* out) {
  for (size_t i = 0; i < n; ++i) {
    out[2 * i] = kHexDigits[in[i] >> 4];
    out[2 * i + 1] = kHexDigits[in[i] & 0x0F];
  }
  out[2 * n] = '\0';
}

bool from_hex(const char* hex, size_t hex_len, uint8_t* out, size_t n) {
  if (hex == nullptr || out == nullptr || hex_len != 2 * n) return false;
  for (size_t i = 0; i < n; ++i) {
    const int high = hex_value(hex[2 * i]);
    const int low = hex_value(hex[2 * i + 1]);
    if (high < 0 || low < 0) return false;
    out[i] = static_cast<uint8_t>((high << 4) | low);
  }
  return true;
}

size_t base64url_encode(const uint8_t* in, size_t n, char* out) {
  size_t written = 0;
  size_t i = 0;
  for (; i + 2 < n; i += 3) {
    const uint32_t chunk = (static_cast<uint32_t>(in[i]) << 16) |
                           (static_cast<uint32_t>(in[i + 1]) << 8) | in[i + 2];
    out[written++] = kBase64Url[(chunk >> 18) & 0x3F];
    out[written++] = kBase64Url[(chunk >> 12) & 0x3F];
    out[written++] = kBase64Url[(chunk >> 6) & 0x3F];
    out[written++] = kBase64Url[chunk & 0x3F];
  }
  const size_t rest = n - i;
  if (rest == 1) {
    const uint32_t chunk = static_cast<uint32_t>(in[i]) << 16;
    out[written++] = kBase64Url[(chunk >> 18) & 0x3F];
    out[written++] = kBase64Url[(chunk >> 12) & 0x3F];
  } else if (rest == 2) {
    const uint32_t chunk = (static_cast<uint32_t>(in[i]) << 16) |
                           (static_cast<uint32_t>(in[i + 1]) << 8);
    out[written++] = kBase64Url[(chunk >> 18) & 0x3F];
    out[written++] = kBase64Url[(chunk >> 12) & 0x3F];
    out[written++] = kBase64Url[(chunk >> 6) & 0x3F];
  }
  out[written] = '\0';
  return written;
}

void format_uuid_v4(const uint8_t random16[16], char* out) {
  uint8_t b[16];
  for (size_t i = 0; i < 16; ++i) b[i] = random16[i];
  b[6] = static_cast<uint8_t>((b[6] & 0x0F) | 0x40);  // version 4
  b[8] = static_cast<uint8_t>((b[8] & 0x3F) | 0x80);  // RFC 4122 variant
  size_t pos = 0;
  for (size_t i = 0; i < 16; ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) out[pos++] = '-';
    out[pos++] = kHexDigits[b[i] >> 4];
    out[pos++] = kHexDigits[b[i] & 0x0F];
  }
  out[pos] = '\0';
}

}  // namespace caudal
