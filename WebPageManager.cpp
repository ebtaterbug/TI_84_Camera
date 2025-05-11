#include "WebPageManager.h"
#include <WiFi.h>
#include <Preferences.h>

WebPageManager::WebPageManager(WiFiManager &manager)
    : server(80),
      wifiManager(manager)
{}

void WebPageManager::begin() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/save", [this]() { handleSave(); });

    // Start access point
    WiFi.softAP("TI84_Config", "TI84Admin");
    Serial.println("Access Point started. Connect to 'TI84_Config' using password 'TI84Admin'.");
    Serial.println("Open '192.168.4.1' in your browser.");

    server.begin();
    Serial.println("Web server started.");
}

void WebPageManager::handleClient() {
    server.handleClient();
}

void WebPageManager::handleRoot() {
    // Form includes SSID, Password, and optional OpenAI Key
    const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Wi-Fi & OpenAI Config</title>
</head>
<body>
    <h1>Wi-Fi and OpenAI Configuration</h1>
    <form action="/save" method="POST">
        SSID: <input type="text" name="ssid"><br><br>
        Password: <input type="text" name="password"><br><br>
        <hr>
        OpenAI Key: <input type="text" name="openaiKey"><br><br>
        <input type="submit" value="Save">
    </form>
    <p>Please enter all fields.</p>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void WebPageManager::handleSave() {
    bool hasSsid      = server.hasArg("ssid");
    bool hasPass      = server.hasArg("password");
    bool hasOpenAIKey = server.hasArg("openaiKey");

    if (hasSsid && hasPass && hasOpenAIKey) {
        String ssid      = server.arg("ssid");
        String pass      = server.arg("password");
        String openaiKey = server.arg("openaiKey");

        // Save Wi-Fi credentials
        wifiManager.saveCredentials(ssid, pass);

        // Save OpenAI Key in the same Preferences namespace
        Preferences prefs;
        prefs.begin("WiFiCreds", false);
        prefs.putString("openAIKey", openaiKey);
        prefs.end();

        Serial.println("All credentials saved:");
        Serial.println("  SSID:      " + ssid);
        Serial.println("  Password:  " + pass);
        Serial.println("  OpenAIKey: " + openaiKey);

        server.send(200, "text/plain", "Credentials saved! Restarting in 3 seconds...");
        Serial.println("Restarting in 3 seconds...");
        delay(3000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Missing SSID, Password, or OpenAI Key.");
    }
}
