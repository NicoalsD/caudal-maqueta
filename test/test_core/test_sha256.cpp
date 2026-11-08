#include <string.h>
#include <unity.h>

#include "caudal/sha256.h"

// Vectors computed with hashlib (Python), not with this code.
void test_sha256_empty_string_matches_protocol(void) {
  char hex[65];
  caudal::sha256_hex(nullptr, 0, hex);
  TEST_ASSERT_EQUAL_STRING("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", hex);
}

void test_sha256_abc(void) {
  char hex[65];
  caudal::sha256_hex(reinterpret_cast<const uint8_t*>("abc"), 3, hex);
  TEST_ASSERT_EQUAL_STRING("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", hex);
}

// Padding boundaries: 55 bytes fits the length in the same block, 56 needs another one.
void test_sha256_padding_boundaries(void) {
  uint8_t data[64];
  memset(data, 'a', sizeof(data));
  char hex[65];
  caudal::sha256_hex(data, 55, hex);
  TEST_ASSERT_EQUAL_STRING("9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318", hex);
  caudal::sha256_hex(data, 56, hex);
  TEST_ASSERT_EQUAL_STRING("b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a", hex);
  caudal::sha256_hex(data, 64, hex);
  TEST_ASSERT_EQUAL_STRING("ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb", hex);
}
