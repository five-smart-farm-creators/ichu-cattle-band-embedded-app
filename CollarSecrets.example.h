#ifndef COLLAR_SECRETS_EXAMPLE_H
#define COLLAR_SECRETS_EXAMPLE_H

// Copy to CollarSecrets.h (git-ignored) and set the real values.
namespace CollarSecrets {

// Must equal DEVICE_API_KEY or DEVELOPMENT_DEVICE_API_KEY registered in the Edge API.
constexpr char kDeviceApiKey[] = "development-device-key";
// Wokwi reaches the host through this gateway name; hardware needs the PC LAN address.
constexpr char kWifiSsid[] = "Wokwi-GUEST";
constexpr char kWifiPassword[] = "";
constexpr char kEdgeBaseUrl[] = "http://host.wokwi.internal:5000";

}  // namespace CollarSecrets

#endif  // COLLAR_SECRETS_EXAMPLE_H
