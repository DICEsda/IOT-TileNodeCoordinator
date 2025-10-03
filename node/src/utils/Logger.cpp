#include "Logger.h"
#include <stdio.h>
#include <stdarg.h>

LogLevel Logger::currentLevel = LogLevel::INFO;
std::function<void(LogLevel, const char*)> Logger::logCallback = nullptr;
char Logger::logBuffer[256];

void Logger::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    while (!Serial && millis() < 3000); // Wait for serial port up to 3 seconds
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::setLogCallback(std::function<void(LogLevel, const char*)> callback) {
    logCallback = callback;
}

void Logger::debug(const char* format, ...) {
    if (currentLevel <= LogLevel::DEBUG) {
        va_list args;
        va_start(args, format);
        log(LogLevel::DEBUG, format, args);
        va_end(args);
    }
}

void Logger::info(const char* format, ...) {
    if (currentLevel <= LogLevel::INFO) {
        va_list args;
        va_start(args, format);
        log(LogLevel::INFO, format, args);
        va_end(args);
    }
}

void Logger::warning(const char* format, ...) {
    if (currentLevel <= LogLevel::WARNING) {
        va_list args;
        va_start(args, format);
        log(LogLevel::WARNING, format, args);
        va_end(args);
    }
}

void Logger::error(const char* format, ...) {
    if (currentLevel <= LogLevel::ERROR) {
        va_list args;
        va_start(args, format);
        log(LogLevel::ERROR, format, args);
        va_end(args);
    }
}

void Logger::logMetric(const char* name, float value) {
    snprintf(logBuffer, sizeof(logBuffer), "METRIC %s=%.3f", name, value);
    if (logCallback) {
        logCallback(LogLevel::INFO, logBuffer);
    }
    Serial.println(logBuffer);
}

void Logger::logEvent(const char* name, const char* data) {
    if (data) {
        snprintf(logBuffer, sizeof(logBuffer), "EVENT %s data=%s", name, data);
    } else {
        snprintf(logBuffer, sizeof(logBuffer), "EVENT %s", name);
    }
    if (logCallback) {
        logCallback(LogLevel::INFO, logBuffer);
    }
    Serial.println(logBuffer);
}

void Logger::logState(const char* component, const char* state) {
    snprintf(logBuffer, sizeof(logBuffer), "STATE %s->%s", component, state);
    if (logCallback) {
        logCallback(LogLevel::INFO, logBuffer);
    }
    Serial.println(logBuffer);
}

void Logger::log(LogLevel level, const char* format, va_list args) {
    uint32_t timestamp = millis();
    int prefixLen = snprintf(logBuffer, sizeof(logBuffer), "[%lu][%s] ", 
                            timestamp, levelToString(level));
    
    vsnprintf(logBuffer + prefixLen, sizeof(logBuffer) - prefixLen, format, args);
    
    if (logCallback) {
        logCallback(level, logBuffer);
    }
    Serial.println(logBuffer);
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
