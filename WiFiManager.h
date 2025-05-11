#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

class WiFiManager {
private:
    void setSuccess(const char* message);
    void setError(const char* message);

public:
    WiFiManager() = default;

    // Load SSID and password from Preferences
    void loadCredentials(String &ssid, String &password);

    // Save SSID and password to Preferences
    void saveCredentials(const String &ssid, const String &password);

    // Connect using given or stored credentials
    void connect();

    // Disconnect from Wi-Fi
    void disconnect();
};

#endif // WIFI_MANAGER_H
