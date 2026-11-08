#include "api_client.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <string.h>

#include "caudal/bodies.h"
#include "caudal/commands.h"
#include "config.h"
#include "secrets.h"

namespace {

constexpr char kTelemetryPath[] = "/api/v1/devices/telemetry";
constexpr char kCommandsPath[] = "/api/v1/devices/commands";

bool is_success(int status) { return status >= 200 && status < 300; }

}  // namespace

void ApiClient::begin() {
  client_.setCACert(API_ROOT_CA);
  client_.setTimeout(HTTP_TIMEOUT_MS / 1000);
  begun_ = true;
}

int ApiClient::send_signed(const DeviceSigner& signer, const char* method, const char* path,
                           const uint8_t* body, size_t len, String* response, int64_t now_ms) {
  if (!begun_) return -1;
  SignedHeaders headers;
  // The signature covers the exact bytes in `body`, so they are sent unchanged.
  if (!signer.sign(method, path, body, len, &headers)) return -2;

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  String url = String(API_BASE_URL) + path;
  if (!http.begin(client_, url)) return -3;
  http.addHeader("X-Device-Id", signer.device_id());
  http.addHeader("X-Timestamp", headers.timestamp);
  http.addHeader("X-Nonce", headers.nonce);
  http.addHeader("X-Signature", headers.signature);

  int status;
  if (strcmp(method, "GET") == 0) {
    status = http.GET();
  } else {
    http.addHeader("Content-Type", "application/json");
    status = http.sendRequest(method, const_cast<uint8_t*>(body), len);
  }
  if (response != nullptr && status > 0) *response = http.getString();
  http.end();

  if (status == 429) backoff_until_ms_ = now_ms + RATE_LIMIT_BACKOFF_MS;
  return status;
}

int ApiClient::post_telemetry(const DeviceSigner& signer, const char* body, size_t len,
                              int64_t now_ms) {
  return send_signed(signer, "POST", kTelemetryPath, reinterpret_cast<const uint8_t*>(body), len,
                     nullptr, now_ms);
}

int ApiClient::get_commands(const DeviceSigner& signer, caudal::ValveCommand* out, size_t cap,
                            size_t* count, size_t* skipped, bool* parsed, int64_t now_ms) {
  *count = 0;
  *skipped = 0;
  *parsed = false;
  String response;
  const int status = send_signed(signer, "GET", kCommandsPath, nullptr, 0, &response, now_ms);
  if (!is_success(status)) return status;
  *parsed = caudal::parse_commands(response.c_str(), response.length(), out, cap, count, skipped);
  return status;
}

int ApiClient::ack(const DeviceSigner& signer, const char* command_id, bool acked,
                   const char* detail, int64_t now_ms) {
  char path[sizeof(kCommandsPath) + 60];
  snprintf(path, sizeof(path), "%s/%s/ack", kCommandsPath, command_id);
  char body[256];
  const size_t len = caudal::build_ack_body(acked, detail, body, sizeof(body));
  if (len == 0) return -4;
  return send_signed(signer, "POST", path, reinterpret_cast<const uint8_t*>(body), len, nullptr,
                     now_ms);
}
