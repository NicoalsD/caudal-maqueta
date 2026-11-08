// Wi-Fi in station mode and clock sync over NTP (signing needs a clock
// within +-300 s, protocol section 3).
#ifndef CAUDAL_WIFI_LINK_H
#define CAUDAL_WIFI_LINK_H

#include <stdint.h>

class WifiLink {
 public:
  void begin();
  // Retries the connection without blocking. Call on every loop pass.
  void update(uint32_t now_ms);
  bool connected() const;
  // True when the clock is after 2024-01-01, so signed requests are allowed.
  bool clock_synced() const;
  bool ready() const { return connected() && clock_synced(); }
  int rssi() const;

 private:
  uint32_t last_attempt_ms_ = 0;
  bool ntp_started_ = false;
};

#endif  // CAUDAL_WIFI_LINK_H
