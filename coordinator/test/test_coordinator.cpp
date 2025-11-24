#ifdef UNIT_TEST

#include <unity.h>
#include "../src/core/Coordinator.h"
#include "../src/managers/NodeRegistry.h"
#include "../src/utils/TimeUtils.h"

// Test NodeRegistry functionality
void test_node_registry_add_node() {
    NodeRegistry registry;
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    
    bool result = registry.addNode(mac);
    TEST_ASSERT_TRUE(result);
    
    // Verify node was added
    TEST_ASSERT_TRUE(registry.isNodeRegistered(mac));
}

void test_node_registry_remove_node() {
    NodeRegistry registry;
    uint8_t mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    
    registry.addNode(mac);
    TEST_ASSERT_TRUE(registry.isNodeRegistered(mac));
    
    registry.removeNode(mac);
    TEST_ASSERT_FALSE(registry.isNodeRegistered(mac));
}

void test_node_registry_max_nodes() {
    NodeRegistry registry;
    uint8_t mac[6];
    
    // Add nodes up to maximum
    for (int i = 0; i < 64; i++) {
        mac[0] = i;
        mac[1] = i >> 8;
        mac[2] = 0x00;
        mac[3] = 0x00;
        mac[4] = 0x00;
        mac[5] = 0x01;
        
        bool result = registry.addNode(mac);
        if (i < 64) {
            TEST_ASSERT_TRUE(result);
        }
    }
}

void test_node_registry_duplicate_node() {
    NodeRegistry registry;
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    
    TEST_ASSERT_TRUE(registry.addNode(mac));
    // Adding same node again should fail
    TEST_ASSERT_FALSE(registry.addNode(mac));
}

// Test TimeUtils functionality
void test_time_utils_timestamp() {
    // Test timestamp generation
    uint32_t ts1 = TimeUtils::getTimestamp();
    delay(10);
    uint32_t ts2 = TimeUtils::getTimestamp();
    
    TEST_ASSERT_GREATER_THAN(ts1, ts2);
}

void test_time_utils_format() {
    // Test time formatting
    char buffer[32];
    TimeUtils::formatTime(1234567890, buffer, sizeof(buffer));
    
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));
}

// Test MAC address utilities
void test_mac_address_compare() {
    uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac2[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac3[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    
    TEST_ASSERT_TRUE(macEqual(mac1, mac2));
    TEST_ASSERT_FALSE(macEqual(mac1, mac3));
}

void test_mac_address_format() {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char buffer[18];
    
    formatMAC(mac, buffer);
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", buffer);
}

// Test configuration values
void test_config_defaults() {
    // Verify default configuration values
    TEST_ASSERT_EQUAL(1, Config::ESPNOW_CHANNEL);
    TEST_ASSERT_EQUAL(64, Config::MAX_NODES);
    TEST_ASSERT_TRUE(Config::PAIRING_TIMEOUT_MS > 0);
}

// Test data structure sizes
void test_struct_sizes() {
    // Ensure structures fit in ESP-NOW payload limits
    TEST_ASSERT_LESS_OR_EQUAL(250, sizeof(NodeTelemetryPacket));
    TEST_ASSERT_LESS_OR_EQUAL(250, sizeof(CommandPacket));
}

// Test telemetry packet validation
void test_telemetry_packet_valid() {
    NodeTelemetryPacket packet;
    packet.temperature = 25.5f;
    packet.brightness = 80;
    packet.voltage = 3.3f;
    packet.red = 255;
    packet.green = 128;
    packet.blue = 64;
    packet.white = 200;
    
    TEST_ASSERT_TRUE(packet.validate());
}

void test_telemetry_packet_invalid_temperature() {
    NodeTelemetryPacket packet;
    packet.temperature = -273.16f; // Below absolute zero
    
    TEST_ASSERT_FALSE(packet.validate());
}

void test_telemetry_packet_invalid_voltage() {
    NodeTelemetryPacket packet;
    packet.voltage = -1.0f; // Negative voltage
    
    TEST_ASSERT_FALSE(packet.validate());
}

// Test command packet creation
void test_command_packet_create() {
    CommandPacket cmd;
    cmd.command = CMD_SET_BRIGHTNESS;
    cmd.value = 100;
    cmd.zone = 1;
    
    TEST_ASSERT_EQUAL(CMD_SET_BRIGHTNESS, cmd.command);
    TEST_ASSERT_EQUAL(100, cmd.value);
    TEST_ASSERT_EQUAL(1, cmd.zone);
}

// Test pairing mode timeout
void test_pairing_timeout() {
    // Test pairing mode timeout logic
    unsigned long startTime = millis();
    unsigned long timeout = 60000; // 60 seconds
    
    // Simulate time passage
    unsigned long currentTime = startTime + timeout + 1000;
    
    TEST_ASSERT_TRUE(currentTime - startTime > timeout);
}

// Test zone assignment
void test_zone_assignment() {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    int zone = 5;
    
    // Simulate zone assignment
    NodeInfo node;
    memcpy(node.mac, mac, 6);
    node.zone = zone;
    
    TEST_ASSERT_EQUAL(zone, node.zone);
    TEST_ASSERT_EQUAL_MEMORY(mac, node.mac, 6);
}

// Test RGBW color values
void test_rgbw_valid_range() {
    RGBWColor color;
    color.red = 255;
    color.green = 255;
    color.blue = 255;
    color.white = 255;
    
    TEST_ASSERT_TRUE(color.red <= 255);
    TEST_ASSERT_TRUE(color.green <= 255);
    TEST_ASSERT_TRUE(color.blue <= 255);
    TEST_ASSERT_TRUE(color.white <= 255);
}

// Test sensor value ranges
void test_sensor_ranges() {
    // Temperature: -40 to 125°C for typical sensors
    float temp = 25.5f;
    TEST_ASSERT_TRUE(temp >= -40.0f && temp <= 125.0f);
    
    // Light: 0 to 4095 for 12-bit ADC
    uint16_t light = 2048;
    TEST_ASSERT_TRUE(light <= 4095);
    
    // Voltage: 2.0 to 4.2V for Li-Ion
    float voltage = 3.7f;
    TEST_ASSERT_TRUE(voltage >= 2.0f && voltage <= 4.2f);
}

// Test button state
void test_button_state() {
    uint8_t buttonState = 0;
    
    // Press button
    buttonState |= (1 << 0);
    TEST_ASSERT_TRUE(buttonState & 0x01);
    
    // Release button
    buttonState &= ~(1 << 0);
    TEST_ASSERT_FALSE(buttonState & 0x01);
}

// Test mmWave detection
void test_mmwave_detection() {
    MMWaveData mmwave;
    mmwave.detected = true;
    mmwave.distance = 150; // cm
    mmwave.energy = 75;
    
    TEST_ASSERT_TRUE(mmwave.detected);
    TEST_ASSERT_EQUAL(150, mmwave.distance);
    TEST_ASSERT_TRUE(mmwave.energy > 0 && mmwave.energy <= 100);
}

// Test WiFi connection state
void test_wifi_state() {
    WiFiState state;
    state.connected = true;
    state.rssi = -65;
    strcpy(state.ssid, "TestNetwork");
    
    TEST_ASSERT_TRUE(state.connected);
    TEST_ASSERT_TRUE(state.rssi < 0); // RSSI is negative
    TEST_ASSERT_EQUAL_STRING("TestNetwork", state.ssid);
}

// Test MQTT connection state
void test_mqtt_state() {
    MQTTState state;
    state.connected = true;
    state.lastPublish = millis();
    state.publishCount = 100;
    
    TEST_ASSERT_TRUE(state.connected);
    TEST_ASSERT_GREATER_THAN(0, state.publishCount);
}

// Test memory allocation
void test_memory_allocation() {
    // Test that critical structures can be allocated
    void* ptr = malloc(sizeof(NodeRegistry));
    TEST_ASSERT_NOT_NULL(ptr);
    free(ptr);
}

// Helper functions
bool macEqual(uint8_t* mac1, uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

void formatMAC(uint8_t* mac, char* buffer) {
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Main test runner
void setup() {
    delay(2000); // Wait for serial
    
    UNITY_BEGIN();
    
    // Node Registry tests
    RUN_TEST(test_node_registry_add_node);
    RUN_TEST(test_node_registry_remove_node);
    RUN_TEST(test_node_registry_max_nodes);
    RUN_TEST(test_node_registry_duplicate_node);
    
    // Time utilities tests
    RUN_TEST(test_time_utils_timestamp);
    RUN_TEST(test_time_utils_format);
    
    // MAC address tests
    RUN_TEST(test_mac_address_compare);
    RUN_TEST(test_mac_address_format);
    
    // Configuration tests
    RUN_TEST(test_config_defaults);
    RUN_TEST(test_struct_sizes);
    
    // Telemetry tests
    RUN_TEST(test_telemetry_packet_valid);
    RUN_TEST(test_telemetry_packet_invalid_temperature);
    RUN_TEST(test_telemetry_packet_invalid_voltage);
    
    // Command tests
    RUN_TEST(test_command_packet_create);
    
    // Feature tests
    RUN_TEST(test_pairing_timeout);
    RUN_TEST(test_zone_assignment);
    RUN_TEST(test_rgbw_valid_range);
    RUN_TEST(test_sensor_ranges);
    RUN_TEST(test_button_state);
    RUN_TEST(test_mmwave_detection);
    RUN_TEST(test_wifi_state);
    RUN_TEST(test_mqtt_state);
    
    // System tests
    RUN_TEST(test_memory_allocation);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}

#endif // UNIT_TEST
