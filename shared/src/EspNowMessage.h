#ifndef ESP_NOW_MESSAGE_H
#define ESP_NOW_MESSAGE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Message types signaled via the 'msg' string field in JSON
enum class MessageType {
	JOIN_REQUEST,
	JOIN_ACCEPT,
	SET_LIGHT,
	NODE_STATUS,
	ERROR,
	ACK
};

// Base message with common helpers
struct EspNowMessage {
	MessageType type;
	String msg;         // e.g. "join_request", "set_light"
	String cmd_id;      // for idempotency/acks (where applicable)
	uint32_t ts;        // timestamp (ms)

	virtual ~EspNowMessage() = default;
	virtual String toJson() const = 0;
	virtual bool fromJson(const String& json) = 0;
};

// Join request message with capability reporting (PRD v0.5)
struct JoinRequestMessage : public EspNowMessage {
	String mac;            // station MAC
	String fw;             // firmware version
	struct Capabilities {
		bool rgbw;         // SK6812B RGBW support
		uint8_t led_count; // pixels per node (default 4)
		bool temp_i2c;     // TMP177 temp sensor via I2C
		bool deep_sleep;   // deep sleep capable
		bool button;       // button input available
	} caps;
	String token;          // rotating token for secure pairing

	JoinRequestMessage();
	String toJson() const override;
	bool fromJson(const String& json) override;
};

// Join accept (coordinator -> node)
struct JoinAcceptMessage : public EspNowMessage {
	String node_id;
	String light_id;
	String lmk;           // link master key (ESP-NOW LMK)
	uint8_t wifi_channel; // WiFi channel coordinator is using
	struct Cfg {
		int pwm_freq;
		int rx_window_ms;
		int rx_period_ms;
	} cfg;

	JoinAcceptMessage();
	String toJson() const override;
	bool fromJson(const String& json) override;
};

// set_light (PRD v0.5)
struct SetLightMessage : public EspNowMessage {
	String light_id;
	// RGBW values (0..255). If omitted, 'value' may be used as brightness fallback.
	uint8_t r = 0, g = 0, b = 0, w = 0;
	uint8_t value = 0; // optional fallback (PWM-like)
	uint16_t fade_ms = 0;
	bool override_status = false;
	uint16_t ttl_ms = 1500;
	String reason;

	SetLightMessage();
	String toJson() const override;
	bool fromJson(const String& json) override;
};

// node_status (PRD v0.5)
struct NodeStatusMessage : public EspNowMessage {
	String node_id;
	String light_id;
	// average output per channel (0..255)
	uint8_t avg_r = 0, avg_g = 0, avg_b = 0, avg_w = 0;
	String status_mode;   // "operational", "pairing", "ota", "error"
	uint16_t vbat_mv = 0;
	float temperature = 0.0f; // temperature in Celsius from TMP177
	bool button_pressed = false; // current button state
	String fw;

	NodeStatusMessage();
	String toJson() const override;
	bool fromJson(const String& json) override;
};

// Error message (minimal)
struct ErrorMessage : public EspNowMessage {
	String node_id;
	String code;
	String info;

	ErrorMessage();
	String toJson() const override;
	bool fromJson(const String& json) override;
};

// Ack for a command id
struct AckMessage : public EspNowMessage {
	AckMessage();
	String toJson() const override;
	bool fromJson(const String& json) override;
};

class MessageFactory {
public:
	static EspNowMessage* createMessage(const String& json);
	static MessageType getMessageType(const String& json);
};

#endif // ESP_NOW_MESSAGE_H

