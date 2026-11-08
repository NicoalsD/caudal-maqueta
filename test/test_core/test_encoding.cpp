#include <string.h>
#include <unity.h>

#include "caudal/encoding.h"

void test_to_hex_writes_lowercase(void) {
  const uint8_t bytes[] = {0x00, 0xAB, 0xFF, 0x10};
  char out[9];
  caudal::to_hex(bytes, sizeof(bytes), out);
  TEST_ASSERT_EQUAL_STRING("00abff10", out);
}

void test_from_hex_round_trip_and_rejects_bad_digits(void) {
  uint8_t out[4];
  TEST_ASSERT_TRUE(caudal::from_hex("00ABff10", 8, out, 4));
  TEST_ASSERT_EQUAL_HEX8(0xAB, out[1]);
  TEST_ASSERT_FALSE(caudal::from_hex("00ABff1g", 8, out, 4));
  TEST_ASSERT_FALSE(caudal::from_hex("00ABff", 6, out, 4));  // length mismatch
}

// RFC 4648 section 10 vectors, in base64url without padding.
void test_base64url_rfc4648_vectors(void) {
  struct Case { const char* in; const char* out; };
  const Case cases[] = {
      {"", ""}, {"f", "Zg"}, {"fo", "Zm8"}, {"foo", "Zm9v"},
      {"foob", "Zm9vYg"}, {"fooba", "Zm9vYmE"}, {"foobar", "Zm9vYmFy"},
  };
  for (const Case& c : cases) {
    char out[16];
    const size_t n = caudal::base64url_encode(reinterpret_cast<const uint8_t*>(c.in),
                                              strlen(c.in), out);
    TEST_ASSERT_EQUAL_STRING(c.out, out);
    TEST_ASSERT_EQUAL_size_t(strlen(c.out), n);
  }
}

void test_base64url_uses_url_alphabet(void) {
  const uint8_t bytes[] = {0xFB, 0xFF};  // encodes to "-_8" in base64url, "+/8" in base64
  char out[8];
  caudal::base64url_encode(bytes, sizeof(bytes), out);
  TEST_ASSERT_EQUAL_STRING("-_8", out);
}

void test_base64url_signature_length_is_86(void) {
  uint8_t sig[64];
  memset(sig, 0xA5, sizeof(sig));
  char out[88];
  TEST_ASSERT_EQUAL_size_t(86, caudal::base64url_encode(sig, sizeof(sig), out));
}

void test_uuid_v4_format(void) {
  uint8_t random[16];
  memset(random, 0xFF, sizeof(random));
  char out[37];
  caudal::format_uuid_v4(random, out);
  TEST_ASSERT_EQUAL_STRING("ffffffff-ffff-4fff-bfff-ffffffffffff", out);
}
