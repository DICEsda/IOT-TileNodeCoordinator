#pragma once

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

// Use Serial (UART) for logger output - more reliable than USB CDC
#define LOG_SERIAL Serial

namespace Logger {
	inline void begin(unsigned long baud) {
		LOG_SERIAL.begin(baud);
		delay(100);
	}

	// Internal helper: print a timestamped, leveled message
	inline void printLevel(const char* level, const char* msg) {
		// Simple ms timestamp (wraps) to help ordering in logs
		unsigned long t = millis();
		LOG_SERIAL.printf("%10lu | %-5s | %s\n", t, level, msg);
	}

	inline void info(const String& msg) { printLevel("INFO", msg.c_str()); }
	inline void warn(const String& msg) { printLevel("WARN", msg.c_str()); }
	inline void error(const String& msg) { printLevel("ERROR", msg.c_str()); }

	inline void info(const char* fmt, ...) {
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLevel("INFO", buf);
	}
	inline void warn(const char* fmt, ...) {
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLevel("WARN", buf);
	}
	inline void error(const char* fmt, ...) {
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLevel("ERROR", buf);
	}

	// alias used in some files
	inline void warning(const char* fmt, ...) {
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLevel("WARN", buf);
	}
}




