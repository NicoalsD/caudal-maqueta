#include "caudal/bodies.h"

#include <string.h>

#include "caudal/iso8601.h"

namespace caudal {

namespace {

// Writes text if it fits. Keeps `pos` and returns false otherwise.
bool put(char* out, size_t cap, size_t* pos, const char* text) {
  const size_t n = strlen(text);
  if (*pos + n + 1 > cap) return false;
  memcpy(out + *pos, text, n);
  *pos += n;
  out[*pos] = '\0';
  return true;
}

bool put_char(char* out, size_t cap, size_t* pos, char c) {
  if (*pos + 2 > cap) return false;
  out[(*pos)++] = c;
  out[*pos] = '\0';
  return true;
}

bool put_escaped(char* out, size_t cap, size_t* pos, const char* text) {
  for (const char* p = text; *p != '\0'; ++p) {
    if (*p == '"' || *p == '\\') {
      if (!put_char(out, cap, pos, '\\')) return false;
    }
    if (!put_char(out, cap, pos, *p)) return false;
  }
  return true;
}

}  // namespace

size_t format_centi(int32_t centi, char* out, size_t cap) {
  char text[16];
  size_t n = 0;
  uint32_t magnitude;
  if (centi < 0) {
    text[n++] = '-';
    magnitude = static_cast<uint32_t>(-(static_cast<int64_t>(centi)));
  } else {
    magnitude = static_cast<uint32_t>(centi);
  }
  char digits[11];
  size_t d = 0;
  uint32_t whole = magnitude / 100;
  do {
    digits[d++] = static_cast<char>('0' + whole % 10);
    whole /= 10;
  } while (whole > 0);
  while (d > 0) text[n++] = digits[--d];
  text[n++] = '.';
  text[n++] = static_cast<char>('0' + (magnitude % 100) / 10);
  text[n++] = static_cast<char>('0' + magnitude % 10);
  text[n] = '\0';
  if (n + 1 > cap) return 0;
  memcpy(out, text, n + 1);
  return n;
}

size_t build_telemetry_body(const TelemetryPoint* points, size_t n, char* out, size_t cap) {
  size_t pos = 0;
  out[0] = '\0';
  if (!put(out, cap, &pos, "{\"points\":[")) return 0;
  for (size_t i = 0; i < n; ++i) {
    char observed[24];
    char value[16];
    if (!format_iso8601_utc(points[i].observed_at_s, observed)) return 0;
    if (format_centi(points[i].value_centi, value, sizeof(value)) == 0) return 0;
    if (i > 0 && !put_char(out, cap, &pos, ',')) return 0;
    if (!put(out, cap, &pos, "{\"id\":\"") || !put(out, cap, &pos, points[i].id) ||
        !put(out, cap, &pos, "\",\"metric\":\"") || !put(out, cap, &pos, points[i].metric) ||
        !put(out, cap, &pos, "\",\"value\":") || !put(out, cap, &pos, value) ||
        !put(out, cap, &pos, ",\"observed_at\":\"") || !put(out, cap, &pos, observed) ||
        !put(out, cap, &pos, "\"}")) {
      return 0;
    }
  }
  if (!put(out, cap, &pos, "]}")) return 0;
  return pos;
}

size_t build_ack_body(bool acked, const char* detail, char* out, size_t cap) {
  size_t pos = 0;
  out[0] = '\0';
  if (!put(out, cap, &pos, acked ? "{\"result\":\"ACKED\",\"detail\":\"" :
                                   "{\"result\":\"FAILED\",\"detail\":\"")) {
    return 0;
  }
  if (!put_escaped(out, cap, &pos, detail)) return 0;
  if (!put(out, cap, &pos, "\"}")) return 0;
  return pos;
}

}  // namespace caudal
