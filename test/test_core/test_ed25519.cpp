#include <string.h>
#include <unity.h>

#include "caudal/canonical.h"
#include "caudal/ed25519.h"
#include "caudal/encoding.h"

namespace {

// RFC 8032, section 7.1, test 1 (empty message).
const char kRfcSeedHex[] = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
const char kRfcPublicHex[] = "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a";
const char kRfcEmptySigHex[] =
    "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701"
    "cf9b46bd25bf5f0595bbe24655141438e7a100b";

// Device vector: seed 0x01..0x20, signing the canonical string of a telemetry POST.
const char kDeviceBody[] =
    "{\"points\":[{\"id\":\"0190f3a2-6666-4000-8000-000000000001\",\"metric\":\"LEVEL\","
    "\"value\":2.10,\"observed_at\":\"2026-10-08T23:00:00Z\"}]}";
const char kDeviceBodySha[] = "2193f654927ab7beeb3a902dfeb1d8bbc8557b7f7670cbfa9d7807b803790685";
const char kDevicePublicB64[] = "ebVWLo_mVPlAeLES6KmLp5AfhTrmlb7X4OORC60ElmQ";
const char kDeviceSigB64[] =
    "0TUNTJU3RGCwV6CWOykPqx3oYOypSkQTVQfGgo9WdY0hKal89hopfQM5hUouVdg6bUvaTxZxIc5xUweE6rs7CA";
const char kDeviceNonce[] = "3f9a1c4e7b2d408f9e6a5c1b2d3e4f50";

void seed_from_hex(const char* hex, uint8_t seed[32]) {
  TEST_ASSERT_TRUE(caudal::from_hex(hex, 64, seed, 32));
}

}  // namespace

void test_ed25519_rfc8032_public_key_and_empty_signature(void) {
  uint8_t seed[32];
  seed_from_hex(kRfcSeedHex, seed);
  caudal::Ed25519Signer signer(seed);

  uint8_t pub[32];
  signer.public_key(pub);
  char pub_hex[65];
  caudal::to_hex(pub, 32, pub_hex);
  TEST_ASSERT_EQUAL_STRING(kRfcPublicHex, pub_hex);

  uint8_t sig[64];
  signer.sign(nullptr, 0, sig);
  char sig_hex[129];
  caudal::to_hex(sig, 64, sig_hex);
  TEST_ASSERT_EQUAL_STRING(kRfcEmptySigHex, sig_hex);
}

void test_ed25519_verify_accepts_own_signature_and_rejects_tampering(void) {
  uint8_t seed[32];
  seed_from_hex(kRfcSeedHex, seed);
  caudal::Ed25519Signer signer(seed);
  uint8_t pub[32];
  signer.public_key(pub);
  uint8_t sig[64];
  const uint8_t msg[] = "hola";
  signer.sign(msg, 4, sig);
  TEST_ASSERT_TRUE(caudal::ed25519_verify(pub, msg, 4, sig));
  const uint8_t tampered[] = "hole";
  TEST_ASSERT_FALSE(caudal::ed25519_verify(pub, tampered, 4, sig));
}

// The full path a device request takes: body hash, canonical string, signature, base64url.
void test_signed_telemetry_request_matches_independent_vector(void) {
  uint8_t seed[32];
  for (int i = 0; i < 32; ++i) seed[i] = static_cast<uint8_t>(i + 1);
  caudal::Ed25519Signer signer(seed);

  uint8_t pub[32];
  signer.public_key(pub);
  char pub_b64[64];
  caudal::base64url_encode(pub, 32, pub_b64);
  TEST_ASSERT_EQUAL_STRING(kDevicePublicB64, pub_b64);

  char body_hash[65];
  caudal::body_sha256_hex(reinterpret_cast<const uint8_t*>(kDeviceBody), strlen(kDeviceBody),
                          body_hash);
  TEST_ASSERT_EQUAL_STRING(kDeviceBodySha, body_hash);

  char canonical[256];
  const size_t n = caudal::build_canonical_string("POST", "/api/v1/devices/telemetry", 1791538200,
                                                  kDeviceNonce, body_hash, canonical,
                                                  sizeof(canonical));
  TEST_ASSERT_EQUAL_size_t(strlen(
      "POST\n/api/v1/devices/telemetry\n1791538200\n"
      "3f9a1c4e7b2d408f9e6a5c1b2d3e4f50\n" "2193f654927ab7beeb3a902dfeb1d8bbc8557b7f7670cbfa9d7807b803790685"),
      n);

  uint8_t sig[64];
  signer.sign(reinterpret_cast<const uint8_t*>(canonical), n, sig);
  char sig_b64[96];
  caudal::base64url_encode(sig, 64, sig_b64);
  TEST_ASSERT_EQUAL_STRING(kDeviceSigB64, sig_b64);
}

void test_canonical_string_rejects_small_buffer(void) {
  char small[20];
  TEST_ASSERT_EQUAL_size_t(0, caudal::build_canonical_string("GET", "/api/v1/devices/commands",
                                                            1, kDeviceNonce,
                                                            kDeviceBodySha, small, sizeof(small)));
}
