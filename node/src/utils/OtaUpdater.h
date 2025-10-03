#pragma once

#include <Arduino.h>
#include <WiFi.h>

class OtaUpdater {
public:
    struct Result {
        bool ok;
        String message;
        int httpCode;
    };

    static bool ensureWifi(const char* ssid, const char* pass, uint32_t timeoutMs = 15000);
    static Result updateFromUrl(const char* url, const char* expectedMd5 = nullptr);
};
