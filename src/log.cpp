#include "coil/log.hpp"
#include <ctime>
#include <cstdarg>
#include <cstring>
#include <thread>
#include <iostream>
#include <unistd.h>

namespace coil {

// Global default logger
std::shared_ptr<Logger> defaultLogger;

Logger::Logger(const std::string& prefix, FILE* stream, LogLevel level)
    : stream_(stream)
    , level_(level)
    , coloredOutput_(isatty(fileno(stream)))
    , prefix_(prefix.empty() ? "COIL" : prefix) {
}

std::shared_ptr<Logger> Logger::create(const std::string& prefix, FILE* stream, LogLevel level) {
    return std::shared_ptr<Logger>(new Logger(prefix, stream, level));
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::setColoredOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    coloredOutput_ = enabled;
}

LogLevel Logger::getLevel() const {
    return level_;
}

bool Logger::isLevelEnabled(LogLevel level) const {
    return level >= level_;
}

std::string Logger::formatMessage(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

Logger::~Logger() {
    // No need to close the stream, as it might be stdout or stderr
}

void initializeLogging() {
    if (!defaultLogger) {
        defaultLogger = Logger::create("COIL", stdout, LogLevel::Info);
    }
}

void cleanupLogging() {
    defaultLogger.reset();
}

} // namespace coil