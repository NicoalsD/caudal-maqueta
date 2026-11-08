// HTTPS client for the backend: telemetry, command polling and ack. Every request is
// signed by the device that owns it (sensor or one of the two actuators).
#ifndef CAUDAL_API_CLIENT_H
#define CAUDAL_API_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#include <WiFiClientSecure.h>

#include "caudal/commands.h"
#include "signer.h"

class ApiClient {
 public:
  void begin();

  // False while a 429 backoff is running.
  bool can_send(int64_t now_ms) const { return now_ms >= backoff_until_ms_; }

  // POST /api/v1/devices/telemetry, signed by `signer`. Returns the HTTP status, or a
  // negative value when the request could not be built or sent.
  int post_telemetry(const DeviceSigner& signer, const char* body, size_t len, int64_t now_ms);

  // GET /api/v1/devices/commands, signed by `signer`: the commands of that device only.
  // `parsed` is false when the body was not the expected JSON (the status can still be 200).
  int get_commands(const DeviceSigner& signer, caudal::ValveCommand* out, size_t cap,
                   size_t* count, size_t* skipped, bool* parsed, int64_t now_ms);

  // POST /api/v1/devices/commands/{id}/ack with {"result","detail"}, signed by `signer`.
  int ack(const DeviceSigner& signer, const char* command_id, bool acked, const char* detail,
          int64_t now_ms);

 private:
  int send_signed(const DeviceSigner& signer, const char* method, const char* path,
                  const uint8_t* body, size_t len, String* response, int64_t now_ms);

  WiFiClientSecure client_;
  bool begun_ = false;
  int64_t backoff_until_ms_ = 0;
};

#endif  // CAUDAL_API_CLIENT_H
