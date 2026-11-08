#include "signer.h"

#include <Arduino.h>
#include <esp_system.h>
#include <string.h>
#include <time.h>

#include "caudal/canonical.h"
#include "caudal/encoding.h"

namespace {
constexpr time_t kMinValidTime = 1704067200;  // 2024-01-01T00:00:00Z
}  // namespace

DeviceSigner::~DeviceSigner() { delete key_; }

void DeviceSigner::begin(const char* device_id, const uint8_t seed[32]) {
  strncpy(device_id_, device_id, sizeof(device_id_) - 1);
  device_id_[sizeof(device_id_) - 1] = '\0';
  delete key_;
  key_ = new caudal::Ed25519Signer(seed);
}

bool DeviceSigner::sign(const char* method, const char* path, const uint8_t* body, size_t len,
                        SignedHeaders* out) const {
  if (key_ == nullptr) return false;
  const time_t now = time(nullptr);
  if (now <= kMinValidTime) return false;

  uint8_t random[16];
  esp_fill_random(random, sizeof(random));
  caudal::to_hex(random, sizeof(random), out->nonce);

  snprintf(out->timestamp, sizeof(out->timestamp), "%lld", static_cast<long long>(now));

  char body_hash[65];
  caudal::body_sha256_hex(body, len, body_hash);

  char canonical[320];
  const size_t canonical_len = caudal::build_canonical_string(
      method, path, static_cast<int64_t>(now), out->nonce, body_hash, canonical, sizeof(canonical));
  if (canonical_len == 0) return false;

  uint8_t signature[64];
  key_->sign(reinterpret_cast<const uint8_t*>(canonical), canonical_len, signature);
  caudal::base64url_encode(signature, sizeof(signature), out->signature);
  return true;
}
