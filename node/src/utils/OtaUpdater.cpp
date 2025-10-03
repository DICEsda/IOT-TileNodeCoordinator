#include "OtaUpdater.h"
#include <HTTPClient.h>
#include <Update.h>

bool OtaUpdater::ensureWifi(const char* ssid, const char* pass, uint32_t timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) return true;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(200);
    }
    return WiFi.status() == WL_CONNECTED;
}

OtaUpdater::Result OtaUpdater::updateFromUrl(const char* url, const char* expectedMd5) {
    Result res{false, "", 0};

    HTTPClient http;
    if (!http.begin(url)) {
        res.message = "HTTP begin failed";
        return res;
    }

    int code = http.GET();
    res.httpCode = code;
    if (code != HTTP_CODE_OK) {
        res.message = String("HTTP GET failed: ") + code;
        http.end();
        return res;
    }

    int len = http.getSize();
    WiFiClient& stream = http.getStream();

    if (!Update.begin(len > 0 ? len : UPDATE_SIZE_UNKNOWN)) {
        res.message = String("Update.begin failed: ") + Update.getError();
        http.end();
        return res;
    }

    if (expectedMd5 && strlen(expectedMd5) > 0) {
        Update.setMD5(expectedMd5);
    }

    size_t written = Update.writeStream(stream);
    if (written == 0) {
        res.message = "No data written";
        http.end();
        Update.abort();
        return res;
    }

    if (!Update.end()) {
        res.message = String("Update.end failed: ") + Update.getError();
        http.end();
        return res;
    }

    http.end();

    if (!Update.isFinished()) {
        res.message = "Update not finished";
        return res;
    }

    res.ok = true;
    res.message = "Update successful, rebooting";
    return res;
}
