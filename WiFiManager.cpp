#include "WiFiManager.h"
#include <WiFi.h>
#include <Preferences.h>

void WiFiManager::setSuccess(const char* message) {
    Serial.print("[SUCCESS] ");
    Serial.println(message);
}

void WiFiManager::setError(const char* message) {
    Serial.print("[ERROR] ");
    Serial.println(message);
}

void WiFiManager::loadCredentials(String &ssid, String &password) {
    Preferences prefs;
    prefs.begin("WiFiCreds", true);  // Read-only mode
    ssid     = prefs.getString("ssid", "");
    password = prefs.getString("password", "");
    prefs.end();

    Serial.println("Loaded credentials:");
    Serial.print("  SSID: ");
    Serial.println(ssid.isEmpty() ? "(none)" : ssid);
    Serial.print("  Password: ");
    Serial.println(password.isEmpty() ? "(none)" : password);
}

void WiFiManager::saveCredentials(const String &ssid, const String &password) {
    Preferences prefs;
    prefs.begin("WiFiCreds", false); // Read/Write mode
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();

    Serial.println("Credentials saved:");
    Serial.println("  SSID: " + ssid);
    Serial.println("  Password: " + password);
}

void WiFiManager::connect() {
    String savedSSID, savedPass;
    loadCredentials(savedSSID, savedPass);
    
    // If SSID is empty, there's nothing to connect to
    if (savedSSID.isEmpty()) {
        setError("No SSID available to connect.");
        return;
    }

    Serial.print("Connecting to: ");
    Serial.println(savedSSID);

    // Connect without password if it's empty
    if (savedPass.isEmpty()) {
        Serial.println("Attempting to connect without a password...");
        WiFi.begin(savedSSID.c_str());
    } else {
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    }

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        setSuccess("Connected to Wi-Fi");
        Serial.print("\nIP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        setError("Failed to connect within timeout");
    }
}



void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    if (WiFi.status() != WL_CONNECTED) {
        setSuccess("Disconnected from Wi-Fi");
    } else {
        setError("Failed to disconnect");
    }
}
