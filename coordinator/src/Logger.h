#pragma once

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

// Use Serial (UART) for logger output - more reliable than USB CDC
#define LOG_SERIAL Serial

namespace Logger {
	enum Level : uint8_t { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };
	namespace { volatile uint8_t gMinLevel = INFO; }

	inline void begin(unsigned long baud) {
		LOG_SERIAL.begin(baud);
		// Give USB CDC time to enumerate; wait up to ~5s if needed
		unsigned long start = millis();
		while (!LOG_SERIAL && (millis() - start) < 5000) { delay(10); }
		// Early preamble to confirm logger path is alive even if printf is buffered
		LOG_SERIAL.println("[LOGGER] started");
	}

	// Internal helper: print a timestamped, leveled message
	inline void printLine(const char* level, const char* msg) {
		// Simple ms timestamp (wraps) to help ordering in logs
		unsigned long t = millis();
		LOG_SERIAL.printf("%10lu | %-5s | %s\n", t, level, msg);
	}

	inline void setMinLevel(Level lvl) { gMinLevel = (uint8_t)lvl; }

	inline void debug(const String& msg) { if (gMinLevel <= DEBUG) printLine("DEBUG", msg.c_str()); }
	inline void info(const String& msg)  { if (gMinLevel <= INFO)  printLine("INFO",  msg.c_str()); }
	inline void warn(const String& msg)  { if (gMinLevel <= WARN)  printLine("WARN",  msg.c_str()); }
	inline void error(const String& msg) { if (gMinLevel <= ERROR) printLine("ERROR", msg.c_str()); }

	inline void debug(const char* fmt, ...) {
		if (gMinLevel > DEBUG) return;
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLine("DEBUG", buf);
	}

	inline void info(const char* fmt, ...) {
		if (gMinLevel > INFO) return;
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLine("INFO", buf);
	}
	inline void warn(const char* fmt, ...) {
		if (gMinLevel > WARN) return;
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLine("WARN", buf);
	}
	inline void error(const char* fmt, ...) {
		if (gMinLevel > ERROR) return;
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLine("ERROR", buf);
	}

	// alias used in some files
	inline void warning(const char* fmt, ...) {
		if (gMinLevel > WARN) return;
		char buf[320];
		va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
		printLine("WARN", buf);
	}

    // Hex dump helper (prints at DEBUG level). Max bytes limited to avoid spam.
    inline void hexDump(const char* tag, const uint8_t* data, size_t len, size_t maxBytes = 64) {
        if (gMinLevel > DEBUG || !data || len == 0) return;
        char line[3 * 64 + 1];
        size_t n = len < maxBytes ? len : maxBytes;
        for (size_t i = 0; i < n; ++i) snprintf(&line[i * 3], 4, "%02X ", data[i]);
        line[n * 3] = '\0';
        char buf[384];
        snprintf(buf, sizeof(buf), "[%s] len=%u data=%s%s", tag ? tag : "HEX", (unsigned)len, line, len > maxBytes ? " ..." : "");
        printLine("DEBUG", buf);
    }
}




