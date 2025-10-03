#pragma once

#include <Arduino.h>

namespace Logger {
	inline void info(const String& msg) { Serial.printf("[INFO] %s\n", msg.c_str()); }
	inline void warn(const String& msg) { Serial.printf("[WARN] %s\n", msg.c_str()); }
	inline void error(const String& msg) { Serial.printf("[ERROR] %s\n", msg.c_str()); }
}




