#ifndef COLLAR_SECRETS_EXAMPLE_H
#define COLLAR_SECRETS_EXAMPLE_H

// Copy to CollarSecrets.h (git-ignored) and set the real values.
namespace CollarSecrets {

// The cloud issues each collar its id when it is registered; this one is the development collar.
constexpr char kDeviceId[] = "9d4a5f3b-3a6e-4f84-8daa-4e5f6a7b8c93";
// Must equal DEVICE_API_KEY or DEVELOPMENT_DEVICE_API_KEY registered in the Edge API.
constexpr char kDeviceApiKey[] = "development-device-key";
// Wokwi reaches the host through this gateway name; hardware needs the PC LAN address.
constexpr char kWifiSsid[] = "Wokwi-GUEST";
constexpr char kWifiPassword[] = "";
constexpr char kEdgeBaseUrl[] = "http://host.wokwi.internal:5000";

}  // namespace CollarSecrets

#endif  // COLLAR_SECRETS_EXAMPLE_H
