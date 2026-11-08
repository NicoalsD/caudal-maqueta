#include "wifi_link.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "secrets.h"

namespace {
// 2024-01-01T00:00:00Z. A clock before this has not synced with NTP yet.
constexpr time_t kMinValidTime = 1704067200;
}  // namespace

void WifiLink::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  last_attempt_ms_ = millis();
}

void WifiLink::update(uint32_t now_ms) {
  if (connected()) {
    if (!ntp_started_) {
      configTime(0, 0, NTP_SERVER);
      ntp_started_ = true;
    }
    return;
  }
  ntp_started_ = false;
  if (now_ms - last_attempt_ms_ < WIFI_RETRY_MS) return;
  last_attempt_ms_ = now_ms;
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool WifiLink::connected() const { return WiFi.status() == WL_CONNECTED; }

bool WifiLink::clock_synced() const { return time(nullptr) > kMinValidTime; }

int WifiLink::rssi() const { return WiFi.RSSI(); }
