#pragma once

#include <Arduino.h>
#include <functional>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void begin(unsigned long baudRate = 115200);
    static void setLogLevel(LogLevel level);
    static void setLogCallback(std::function<void(LogLevel, const char*)> callback);

    static void debug(const char* format, ...);
    static void info(const char* format, ...);
    static void warning(const char* format, ...);
    static void error(const char* format, ...);

    // Structured logging methods
    static void logMetric(const char* name, float value);
    static void logEvent(const char* name, const char* data = nullptr);
    static void logState(const char* component, const char* state);

private:
    static LogLevel currentLevel;
    static std::function<void(LogLevel, const char*)> logCallback;
    static void log(LogLevel level, const char* format, va_list args);
    
    static const char* levelToString(LogLevel level);
    static char logBuffer[256]; // Buffer for formatting log messages
};
