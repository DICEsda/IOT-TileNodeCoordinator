#include "EspNowMessage.h"

// JoinRequestMessage implementation
JoinRequestMessage::JoinRequestMessage() {
    type = MessageType::JOIN_REQUEST;
    msg = "join_request";
    timestamp = millis();
}

String JoinRequestMessage::toJson() const {
    DynamicJsonDocument doc(512);
    doc["msg"] = msg;
    doc["mac"] = mac;
    doc["caps"]["pwm"] = caps.pwm;
    doc["caps"]["temp_spi"] = caps.temp_spi;
    doc["fw"] = fw;
    doc["token"] = token;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool JoinRequestMessage::fromJson(const String& json) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return false;
    
    msg = doc["msg"].as<String>();
    mac = doc["mac"].as<String>();
    caps.pwm = doc["caps"]["pwm"].as<bool>();
    caps.temp_spi = doc["caps"]["temp_spi"].as<bool>();
    fw = doc["fw"].as<String>();
    token = doc["token"].as<String>();
    
    return true;
}

// JoinAcceptMessage implementation
JoinAcceptMessage::JoinAcceptMessage() {
    type = MessageType::JOIN_ACCEPT;
    msg = "join_accept";
    timestamp = millis();
}

String JoinAcceptMessage::toJson() const {
    DynamicJsonDocument doc(512);
    doc["msg"] = msg;
    doc["node_id"] = node_id;
    doc["lmk"] = lmk;
    doc["light_id"] = light_id;
    doc["cfg"]["pwm_freq"] = cfg.pwm_freq;
    doc["cfg"]["rx_window_ms"] = cfg.rx_window_ms;
    doc["cfg"]["rx_period_ms"] = cfg.rx_period_ms;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool JoinAcceptMessage::fromJson(const String& json) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return false;
    
    msg = doc["msg"].as<String>();
    node_id = doc["node_id"].as<String>();
    lmk = doc["lmk"].as<String>();
    light_id = doc["light_id"].as<String>();
    cfg.pwm_freq = doc["cfg"]["pwm_freq"].as<int>();
    cfg.rx_window_ms = doc["cfg"]["rx_window_ms"].as<int>();
    cfg.rx_period_ms = doc["cfg"]["rx_period_ms"].as<int>();
    
    return true;
}

// SetLightMessage implementation
SetLightMessage::SetLightMessage() {
    type = MessageType::SET_LIGHT;
    msg = "set_light";
    timestamp = millis();
}

String SetLightMessage::toJson() const {
    DynamicJsonDocument doc(512);
    doc["msg"] = msg;
    doc["cmd_id"] = cmd_id;
    doc["light_id"] = light_id;
    doc["value"] = value;
    doc["fade_ms"] = fade_ms;
    doc["reason"] = reason;
    doc["ttl_ms"] = ttl_ms;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool SetLightMessage::fromJson(const String& json) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return false;
    
    msg = doc["msg"].as<String>();
    cmd_id = doc["cmd_id"].as<String>();
    light_id = doc["light_id"].as<String>();
    value = doc["value"].as<uint8_t>();
    fade_ms = doc["fade_ms"].as<uint16_t>();
    reason = doc["reason"].as<String>();
    ttl_ms = doc["ttl_ms"].as<uint16_t>();
    
    return true;
}

// NodeStatusMessage implementation
NodeStatusMessage::NodeStatusMessage() {
    type = MessageType::NODE_STATUS;
    msg = "node_status";
    timestamp = millis();
}

String NodeStatusMessage::toJson() const {
    DynamicJsonDocument doc(512);
    doc["msg"] = msg;
    doc["node_id"] = node_id;
    doc["light_id"] = light_id;
    doc["temp_c"] = temp_c;
    doc["duty"] = duty;
    doc["fw"] = fw;
    doc["vbat_mv"] = vbat_mv;
    doc["ts"] = timestamp;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool NodeStatusMessage::fromJson(const String& json) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return false;
    
    msg = doc["msg"].as<String>();
    node_id = doc["node_id"].as<String>();
    light_id = doc["light_id"].as<String>();
    temp_c = doc["temp_c"].as<float>();
    duty = doc["duty"].as<uint8_t>();
    fw = doc["fw"].as<String>();
    vbat_mv = doc["vbat_mv"].as<uint16_t>();
    timestamp = doc["ts"].as<uint32_t>();
    
    return true;
}

// ErrorMessage implementation
ErrorMessage::ErrorMessage() {
    type = MessageType::ERROR;
    msg = "error";
    timestamp = millis();
}

String ErrorMessage::toJson() const {
    DynamicJsonDocument doc(256);
    doc["msg"] = msg;
    doc["node_id"] = node_id;
    doc["code"] = code;
    doc["info"] = info;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool ErrorMessage::fromJson(const String& json) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return false;
    
    msg = doc["msg"].as<String>();
    node_id = doc["node_id"].as<String>();
    code = doc["code"].as<String>();
    info = doc["info"].as<String>();
    
    return true;
}

// AckMessage implementation
AckMessage::AckMessage() {
    type = MessageType::ACK;
    msg = "ack";
    timestamp = millis();
}

String AckMessage::toJson() const {
    DynamicJsonDocument doc(128);
    doc["msg"] = msg;
    doc["cmd_id"] = cmd_id;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool AckMessage::fromJson(const String& json) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return false;
    
    msg = doc["msg"].as<String>();
    cmd_id = doc["cmd_id"].as<String>();
    
    return true;
}

// MessageFactory implementation
EspNowMessage* MessageFactory::createMessage(const String& json) {
    MessageType type = getMessageType(json);
    
    switch (type) {
        case MessageType::JOIN_REQUEST:
            return new JoinRequestMessage();
        case MessageType::JOIN_ACCEPT:
            return new JoinAcceptMessage();
        case MessageType::SET_LIGHT:
            return new SetLightMessage();
        case MessageType::NODE_STATUS:
            return new NodeStatusMessage();
        case MessageType::ERROR:
            return new ErrorMessage();
        case MessageType::ACK:
            return new AckMessage();
        default:
            return nullptr;
    }
}

MessageType MessageFactory::getMessageType(const String& json) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) return MessageType::ERROR;
    
    String msg = doc["msg"].as<String>();
    
    if (msg == "join_request") return MessageType::JOIN_REQUEST;
    if (msg == "join_accept") return MessageType::JOIN_ACCEPT;
    if (msg == "set_light") return MessageType::SET_LIGHT;
    if (msg == "node_status") return MessageType::NODE_STATUS;
    if (msg == "error") return MessageType::ERROR;
    if (msg == "ack") return MessageType::ACK;
    
    return MessageType::ERROR;
}

