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

// ANSI color codes
static const char* levelColors[] = {
    "\x1b[90m",  // TRACE: Bright Black
    "\x1b[36m",  // DEBUG: Cyan
    "\x1b[32m",  // INFO: Green
    "\x1b[33m",  // WARNING: Yellow
    "\x1b[31m",  // ERROR: Red
    "\x1b[35m",  // FATAL: Magenta
    ""           // NONE: No color
};

static const char* levelNames[] = {
    "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"
};

static const char* resetColor = "\x1b[0m";

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

template<typename... Args>
void Logger::log(LogLevel level, const char* file, int line, const char* func, const char* fmt, Args&&... args) {
    if (level < level_) return;
    
    char timestamp[32];
    std::time_t now = std::time(nullptr);
    std::tm* tm_info = std::localtime(&now);
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Print timestamp, level, and thread id
    if (coloredOutput_) {
        fprintf(stream_, "%s [%s%s%s] [%s] [%lu] ", 
                timestamp, 
                levelColors[static_cast<int>(level)], 
                levelNames[static_cast<int>(level)], 
                resetColor,
                prefix_.c_str(),
                static_cast<unsigned long>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    } else {
        fprintf(stream_, "%s [%s] [%s] [%lu] ", 
                timestamp, 
                levelNames[static_cast<int>(level)],
                prefix_.c_str(),
                static_cast<unsigned long>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    }
    
    // Print source location for debug and higher
    if (level >= LogLevel::Debug) {
        fprintf(stream_, "(%s:%d:%s) ", file, line, func);
    }
    
    // Print the actual message
    fprintf(stream_, fmt, std::forward<Args>(args)...);
    fprintf(stream_, "\n");
    fflush(stream_);
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