// Secrets template for the rig. Copy to include/secrets.h (ignored by git) and fill in.
// Never commit secrets.h.
#ifndef CAUDAL_SECRETS_H
#define CAUDAL_SECRETS_H

// Wi-Fi network (2.4 GHz).
#define WIFI_SSID "network-name"
#define WIFI_PASSWORD "network-password"

// Backend origin, without the path. The firmware adds /api/v1/... itself.
#define API_BASE_URL "https://api.example.invalid"

// Root CA certificate of the backend, PEM format, used for HTTPS.
#define API_ROOT_CA \
  "-----BEGIN CERTIFICATE-----\n" \
  "(paste the root certificate here)\n" \
  "-----END CERTIFICATE-----\n"

// The rig is three signed devices, as the backend models them: one LEVEL_SENSOR for the
// tank and one VALVE_ACTUATOR per outlet. Each device has its own UUID and its own Ed25519
// seed (32 bytes, 64 hex digits). The seed is the private key and never leaves the device.
// Register each public key with POST /api/v1/devices/{id}/keys.
#define SENSOR_DEVICE_ID "00000000-0000-7000-8000-000000000000"
#define SENSOR_SEED_HEX "0000000000000000000000000000000000000000000000000000000000000000"

// Actuator 1 moves valve 1 (valve UUID from org.valves). Commands for valve 1 are polled
// and acked with this device.
#define VALVE_1_DEVICE_ID "00000000-0000-7000-8000-000000000011"
#define VALVE_1_SEED_HEX "1111111111111111111111111111111111111111111111111111111111111111"
#define VALVE_1_ID "00000000-0000-7000-8000-000000000001"

// Actuator 2 moves valve 2.
#define VALVE_2_DEVICE_ID "00000000-0000-7000-8000-000000000012"
#define VALVE_2_SEED_HEX "2222222222222222222222222222222222222222222222222222222222222222"
#define VALVE_2_ID "00000000-0000-7000-8000-000000000002"

#endif  // CAUDAL_SECRETS_H
