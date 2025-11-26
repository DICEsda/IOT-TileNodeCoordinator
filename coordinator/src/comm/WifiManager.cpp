#include "WifiManager.h"
#include "../utils/Logger.h"
#include "EspNow.h"

WifiManager::WifiManager()
    : config("wifi")
    , lastReconnectAttempt(0) {
    status = Status{};
}

bool WifiManager::begin() {
    if (!config.begin()) {
        Logger::warn("WiFi config namespace missing; creating new store");
    }

    storedSsid = config.getString("ssid", "");
    storedPassword = config.getString("password", "");

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    // Try stored credentials first
    if (!storedSsid.isEmpty()) {
        Serial.printf("Found stored Wi-Fi: %s\n", storedSsid.c_str());
        if (attemptConnect(storedSsid, storedPassword)) {
            return true;
        }
        // Connection failed with stored credentials
        Serial.println("✗ Stored credentials failed to connect.");
        Serial.println("Would you like to:");
        Serial.println("  1) Retry existing credentials");
        Serial.println("  2) Configure new Wi-Fi network");
        Serial.println("  3) Continue offline");
        Serial.print("Enter choice (1-3): ");
        String choice = promptLine("", false);
        choice.trim();
        
        if (choice == "1") {
            Serial.println("Retrying stored credentials in background...");
            // Will retry in loop()
            return false;
        } else if (choice == "3") {
            status.offlineMode = true;
            Serial.println("Continuing in offline mode. Use serial menu to configure later.");
            return false;
        }
        // Fall through to interactive setup for choice "2" or invalid input
    } else {
        // No stored credentials
        Serial.println("═══════════════════════════════════════");
        Serial.println("  No Wi-Fi credentials configured");
        Serial.println("═══════════════════════════════════════");
    }

    Serial.println("Configure Wi-Fi? (y/n)");
    if (!promptYesNo("") ) {
        status.offlineMode = true;
        Serial.println("Continuing in offline mode. MQTT will retry when Wi-Fi becomes available.");
        return false;
    }

    bool configured = interactiveSetup();
    if (!configured) {
        status.offlineMode = true;
        Serial.println("Wi-Fi setup skipped. Running offline.");
        return false;
    }
    return true;
}

void WifiManager::loop() {
    if (status.offlineMode || storedSsid.isEmpty()) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        uint32_t now = millis();
        if (now - lastReconnectAttempt > 10000) {
            lastReconnectAttempt = now;
            attemptConnect(storedSsid, storedPassword, false);
        }
    } else {
        updateStatusCache();
    }
}

bool WifiManager::ensureConnected() {
    if (status.offlineMode) {
        return false;
    }
    if (WiFi.status() == WL_CONNECTED) {
        updateStatusCache();
        return true;
    }
    if (storedSsid.isEmpty()) {
        return false;
    }
    return attemptConnect(storedSsid, storedPassword);
}

bool WifiManager::attemptConnect(const String& ssid, const String& password, bool verbose) {
    if (ssid.isEmpty()) {
        return false;
    }

    if (verbose) {
        Serial.printf("Connecting to Wi-Fi SSID '%s'...\n", ssid.c_str());
    }
    Logger::info("Connecting to Wi-Fi: %s", ssid.c_str());

    // CRITICAL: Use WiFi.disconnect(false, false) to avoid deinitializing ESP-NOW
    // The second parameter (true) would erase WiFi config AND deinit radio, breaking ESP-NOW!
    WiFi.disconnect(false, false);
    delay(100);
    WiFi.begin(ssid.c_str(), password.c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
        delay(500);
        if (verbose) Serial.print('.');
    }
    if (verbose) Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        updateStatusCache();
        status.offlineMode = false;
        storedSsid = ssid;
        storedPassword = password;
        config.setString("ssid", storedSsid);
        config.setString("password", storedPassword);
        Serial.printf("✓ Wi-Fi connected: %s (IP %s, RSSI %d)\n",
                      status.ssid.c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
        Logger::info("Wi-Fi connected: %s", status.ssid.c_str());
        
        if (espNow) {
            espNow->updatePeerChannels();
        }
        return true;
    }

    Logger::warn("Wi-Fi connection to %s failed", ssid.c_str());
    Serial.println("✗ Failed to connect. Check credentials and retry.");
    return false;
}

bool WifiManager::interactiveSetup() {
    while (true) {
        String chosenSsid;
        if (!selectNetwork(chosenSsid)) {
            Serial.println("No networks selected. Retry? (y/n)");
            if (!promptYesNo("")) {
                return false;
            }
            continue;
        }

        String password = promptLine("Enter password (leave empty for open network): ", true);
        if (attemptConnect(chosenSsid, password)) {
            return true;
        }

        Serial.println("Connection failed. Try a different network? (y/n)");
        if (!promptYesNo("")) {
            return false;
        }
    }
}

bool WifiManager::selectNetwork(String& ssidOut) {
    Serial.println("Scanning for Wi-Fi networks...");
    int count = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
    if (count <= 0) {
        Serial.println("No networks found. Enter SSID manually? (y/n)");
        if (promptYesNo("")) {
            ssidOut = promptLine("Enter SSID: ", false);
            return !ssidOut.isEmpty();
        }
        return false;
    }

    for (int i = 0; i < count; ++i) {
        Serial.printf("[%d] %s (RSSI %d dBm)%s\n", i, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                     WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " [open]" : "");
    }
    Serial.println("Enter the index of the network to use, or type the SSID manually:");
    String choice = promptLine("> ", false);
    choice.trim();

    bool numeric = true;
    for (size_t i = 0; i < choice.length(); ++i) {
        if (!isDigit(choice[i])) {
            numeric = false;
            break;
        }
    }

    if (numeric && choice.length() > 0) {
        int idx = choice.toInt();
        if (idx >= 0 && idx < count) {
            ssidOut = WiFi.SSID(idx);
            WiFi.scanDelete();
            return true;
        }
    }

    ssidOut = choice;
    WiFi.scanDelete();
    return !ssidOut.isEmpty();
}

bool WifiManager::promptYesNo(const String& prompt) {
    String line = promptLine(prompt.isEmpty() ? "(y/n): " : prompt, false);
    line.toLowerCase();
    return line == "y" || line == "yes";
}

String WifiManager::promptLine(const String& prompt, bool allowEmpty, bool /*hide*/) {
    if (!prompt.isEmpty()) {
        Serial.print(prompt);
    }
    Serial.flush();
    String input;
    while (!Serial.available()) {
        delay(10);
    }
    input = Serial.readStringUntil('\n');
    input.trim();
    if (!allowEmpty) {
        while (input.isEmpty()) {
            Serial.print("> ");
            Serial.flush();
            while (!Serial.available()) {
                delay(10);
            }
            input = Serial.readStringUntil('\n');
            input.trim();
        }
    }
    return input;
}

void WifiManager::updateStatusCache() {
    status.connected = (WiFi.status() == WL_CONNECTED);
    status.ssid = status.connected ? WiFi.SSID() : "";
    status.rssi = status.connected ? WiFi.RSSI() : -127;
    status.ip = status.connected ? WiFi.localIP() : IPAddress();
}

bool WifiManager::reconfigureWifi() {
    Serial.println("═══════════════════════════════════════");
    Serial.println("  Wi-Fi Reconfiguration");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Currently connected to: %s\n", WiFi.SSID().c_str());
        Serial.println("This will disconnect and configure a new network.");
        Serial.println("Continue? (y/n)");
        if (!promptYesNo("")) {
            Serial.println("Reconfiguration cancelled.");
            return false;
        }
        WiFi.disconnect(false, false);
        delay(500);
    }
    
    bool configured = interactiveSetup();
    if (configured) {
        status.offlineMode = false;
        Serial.println("✓ Wi-Fi reconfigured successfully!");
        return true;
    } else {
        Serial.println("✗ Wi-Fi reconfiguration failed or cancelled.");
        return false;
    }
}
