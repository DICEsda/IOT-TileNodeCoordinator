#include "EspNowMessage.h"

// --- JoinRequest ---
JoinRequestMessage::JoinRequestMessage() {
	type = MessageType::JOIN_REQUEST;
	msg = "join_request";
	ts = millis();
}

String JoinRequestMessage::toJson() const {
	DynamicJsonDocument doc(512);
	doc["msg"] = msg;
	doc["mac"] = mac;
	doc["fw"] = fw;
	doc["caps"]["rgbw"] = caps.rgbw;
	doc["caps"]["led_count"] = caps.led_count;
	doc["caps"]["temp_i2c"] = caps.temp_i2c;
	doc["caps"]["deep_sleep"] = caps.deep_sleep;
	doc["caps"]["button"] = caps.button;
	doc["token"] = token;
	String out; serializeJson(doc, out); return out;
}

bool JoinRequestMessage::fromJson(const String& json) {
	DynamicJsonDocument doc(512);
	DeserializationError err = deserializeJson(doc, json);
	if (err) return false;
	msg = doc["msg"].as<String>();
	mac = doc["mac"].as<String>();
	fw = doc["fw"].as<String>();
	caps.rgbw = doc["caps"]["rgbw"].as<bool>();
	caps.led_count = doc["caps"]["led_count"].as<uint8_t>();
	caps.temp_i2c = doc["caps"]["temp_i2c"] | false;
	caps.deep_sleep = doc["caps"]["deep_sleep"].as<bool>();
	caps.button = doc["caps"]["button"] | false;
	token = doc["token"].as<String>();
	return true;
}

// --- JoinAccept ---
JoinAcceptMessage::JoinAcceptMessage() {
	type = MessageType::JOIN_ACCEPT;
	msg = "join_accept";
	ts = millis();
	wifi_channel = 1; // Default to channel 1
	// Initialize config struct with defaults
	cfg.pwm_freq = 0;
	cfg.rx_window_ms = 20;
	cfg.rx_period_ms = 100;
}

String JoinAcceptMessage::toJson() const {
	DynamicJsonDocument doc(256);
	doc["msg"] = msg;
	doc["node_id"] = node_id;
	doc["light_id"] = light_id;
	doc["lmk"] = lmk;
	doc["wifi_channel"] = wifi_channel;
	doc["cfg"]["pwm_freq"] = cfg.pwm_freq;
	doc["cfg"]["rx_window_ms"] = cfg.rx_window_ms;
	doc["cfg"]["rx_period_ms"] = cfg.rx_period_ms;
	String out; serializeJson(doc, out); return out;
}

bool JoinAcceptMessage::fromJson(const String& json) {
	DynamicJsonDocument doc(768); // Increased to handle full join_accept with nested config
	DeserializationError err = deserializeJson(doc, json);
	if (err) {
		Serial.printf("JoinAccept parse error: %s (buffer may be too small)\n", err.c_str());
		Serial.printf("  Message length: %d bytes\n", json.length());
		return false;
	}
	msg = doc["msg"].as<String>();
	node_id = doc["node_id"].as<String>();
	light_id = doc["light_id"].as<String>();
	lmk = doc["lmk"].as<String>();
	wifi_channel = doc["wifi_channel"] | 1; // Default to 1 if missing
	cfg.pwm_freq = doc["cfg"]["pwm_freq"].as<int>();
	cfg.rx_window_ms = doc["cfg"]["rx_window_ms"].as<int>();
	cfg.rx_period_ms = doc["cfg"]["rx_period_ms"].as<int>();
	return true;
}

// --- SetLight ---
SetLightMessage::SetLightMessage() {
	type = MessageType::SET_LIGHT;
	msg = "set_light";
	ts = millis();
}

String SetLightMessage::toJson() const {
	DynamicJsonDocument doc(256);
	doc["msg"] = msg;
	doc["cmd_id"] = cmd_id;
	doc["light_id"] = light_id;
	doc["r"] = r; doc["g"] = g; doc["b"] = b; doc["w"] = w;
	doc["value"] = value;
	doc["fade_ms"] = fade_ms;
	doc["override_status"] = override_status;
	doc["ttl_ms"] = ttl_ms;
	if (reason.length()) doc["reason"] = reason;
	String out; serializeJson(doc, out); return out;
}

bool SetLightMessage::fromJson(const String& json) {
	DynamicJsonDocument doc(256);
	DeserializationError err = deserializeJson(doc, json);
	if (err) return false;
	msg = doc["msg"].as<String>();
	cmd_id = doc["cmd_id"].as<String>();
	light_id = doc["light_id"].as<String>();
	// Fix: Use ternary operator or explicit checks instead of logical OR for default values
	r = doc.containsKey("r") ? doc["r"].as<uint8_t>() : 0;
	g = doc.containsKey("g") ? doc["g"].as<uint8_t>() : 0;
	b = doc.containsKey("b") ? doc["b"].as<uint8_t>() : 0;
	w = doc.containsKey("w") ? doc["w"].as<uint8_t>() : 0;
	value = doc.containsKey("value") ? doc["value"].as<uint8_t>() : 0;
	fade_ms = doc.containsKey("fade_ms") ? doc["fade_ms"].as<uint16_t>() : 0;
	override_status = doc["override_status"] | false;
	ttl_ms = doc.containsKey("ttl_ms") ? doc["ttl_ms"].as<uint16_t>() : 1500;
	reason = doc["reason"].as<String>();
	return true;
}

// --- NodeStatus ---
NodeStatusMessage::NodeStatusMessage() {
	type = MessageType::NODE_STATUS;
	msg = "node_status";
	ts = millis();
}

String NodeStatusMessage::toJson() const {
	DynamicJsonDocument doc(256);
	doc["msg"] = msg;
	doc["node_id"] = node_id;
	doc["light_id"] = light_id;
	doc["avg_r"] = avg_r;
	doc["avg_g"] = avg_g;
	doc["avg_b"] = avg_b;
	doc["avg_w"] = avg_w;
	doc["status_mode"] = status_mode;
	doc["vbat_mv"] = vbat_mv;
	doc["temperature"] = temperature;
	doc["button_pressed"] = button_pressed;
	doc["fw"] = fw;
	doc["ts"] = ts;
	String out; serializeJson(doc, out); return out;
}

bool NodeStatusMessage::fromJson(const String& json) {
	DynamicJsonDocument doc(256);
	DeserializationError err = deserializeJson(doc, json);
	if (err) return false;
	msg = doc["msg"].as<String>();
	node_id = doc["node_id"].as<String>();
	light_id = doc["light_id"].as<String>();
	// Fix: Use proper default value checks
	avg_r = doc.containsKey("avg_r") ? doc["avg_r"].as<uint8_t>() : 0;
	avg_g = doc.containsKey("avg_g") ? doc["avg_g"].as<uint8_t>() : 0;
	avg_b = doc.containsKey("avg_b") ? doc["avg_b"].as<uint8_t>() : 0;
	avg_w = doc.containsKey("avg_w") ? doc["avg_w"].as<uint8_t>() : 0;
	status_mode = doc["status_mode"].as<String>();
	vbat_mv = doc.containsKey("vbat_mv") ? doc["vbat_mv"].as<uint16_t>() : 0;
	temperature = doc.containsKey("temperature") ? doc["temperature"].as<float>() : 0.0f;
	button_pressed = doc.containsKey("button_pressed") ? doc["button_pressed"].as<bool>() : false;
	fw = doc["fw"].as<String>();
	ts = doc.containsKey("ts") ? doc["ts"].as<uint32_t>() : millis();
	return true;
}

// --- Error ---
ErrorMessage::ErrorMessage() {
	type = MessageType::ERROR;
	msg = "error";
	ts = millis();
}

String ErrorMessage::toJson() const {
	DynamicJsonDocument doc(192);
	doc["msg"] = msg;
	doc["node_id"] = node_id;
	doc["code"] = code;
	doc["info"] = info;
	String out; serializeJson(doc, out); return out;
}

bool ErrorMessage::fromJson(const String& json) {
	DynamicJsonDocument doc(192);
	DeserializationError err = deserializeJson(doc, json);
	if (err) return false;
	msg = doc["msg"].as<String>();
	node_id = doc["node_id"].as<String>();
	code = doc["code"].as<String>();
	info = doc["info"].as<String>();
	return true;
}

// --- Ack ---
AckMessage::AckMessage() {
	type = MessageType::ACK;
	msg = "ack";
	ts = millis();
}

String AckMessage::toJson() const {
	DynamicJsonDocument doc(96);
	doc["msg"] = msg;
	doc["cmd_id"] = cmd_id;
	String out; serializeJson(doc, out); return out;
}

bool AckMessage::fromJson(const String& json) {
	DynamicJsonDocument doc(96);
	DeserializationError err = deserializeJson(doc, json);
	if (err) return false;
	msg = doc["msg"].as<String>();
	cmd_id = doc["cmd_id"].as<String>();
	return true;
}

// --- Factory ---
EspNowMessage* MessageFactory::createMessage(const String& json) {
	MessageType t = getMessageType(json);
	EspNowMessage* m = nullptr;
	switch (t) {
		case MessageType::JOIN_REQUEST: m = new JoinRequestMessage(); break;
		case MessageType::JOIN_ACCEPT:  m = new JoinAcceptMessage(); break;
		case MessageType::SET_LIGHT:    m = new SetLightMessage(); break;
		case MessageType::NODE_STATUS:  m = new NodeStatusMessage(); break;
		case MessageType::ERROR:        m = new ErrorMessage(); break;
		case MessageType::ACK:          m = new AckMessage(); break;
		default: return nullptr;
	}
	if (m && !m->fromJson(json)) { delete m; return nullptr; }
	return m;
}

MessageType MessageFactory::getMessageType(const String& json) {
	// Use filter to only extract "msg" field - more memory efficient
	DynamicJsonDocument filter(48);
	filter["msg"] = true;
	
	DynamicJsonDocument doc(256);
	DeserializationError err = deserializeJson(doc, json, DeserializationOption::Filter(filter));
	if (err) {
		Serial.printf("MessageFactory: Failed to parse message type: %s\n", err.c_str());
		return MessageType::ERROR;
	}
	
	String m = doc["msg"].as<String>();
	if (m == "join_request") return MessageType::JOIN_REQUEST;
	if (m == "join_accept") return MessageType::JOIN_ACCEPT;
	if (m == "set_light") return MessageType::SET_LIGHT;
	if (m == "node_status") return MessageType::NODE_STATUS;
	if (m == "ack") return MessageType::ACK;
	return MessageType::ERROR;
}

