#ifndef WEB_PAGE_MANAGER_H
#define WEB_PAGE_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include "WiFiManager.h"

class WebPageManager {
private:
    WebServer server;
    WiFiManager &wifiManager;

    void handleRoot();
    void handleSave();

public:
    WebPageManager(WiFiManager &manager);

    void begin();
    void handleClient();
};

#endif // WEB_PAGE_MANAGER_H
