#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <cstdio>
#include <vector>
#include <chrono>
#include <string_view>
#include <functional>

namespace coil {

/**
 * @brief Logging levels
 */
enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    None
};

/**
 * @brief Modern C++ Logger class
 */
class Logger {
public:
    /**
     * @brief Construct a logger with a specific stream
     * 
     * @param prefix Logger name/prefix
     * @param stream Output stream (stdout, file, etc.)
     * @param level Initial log level
     */
    Logger(const std::string& prefix, FILE* stream, LogLevel level);
    
    /**
     * @brief Create a logger with default settings
     * 
     * @return std::shared_ptr<Logger> 
     */
    static std::shared_ptr<Logger> create(const std::string& prefix, FILE* stream, LogLevel level);
    
    /**
     * @brief Set the log level
     * 
     * @param level New log level
     */
    void setLevel(LogLevel level);
    
    /**
     * @brief Enable or disable colored output
     * 
     * @param enabled Whether to use ANSI colors
     */
    void setColoredOutput(bool enabled);
    
    /**
     * @brief Log a message
     * 
     * @param level Log level
     * @param file Source file
     * @param line Line number
     * @param func Function name
     * @param fmt Format string
     * @param args Format arguments
     */
    template<typename... Args>
    void log(LogLevel level, const char* file, int line, const char* func, const char* fmt, Args&&... args);
    
    /**
     * @brief Get the current log level
     * 
     * @return LogLevel 
     */
    LogLevel getLevel() const;
    
    /**
     * @brief Check if a level is enabled
     * 
     * @param level Level to check
     * @return true if enabled
     */
    bool isLevelEnabled(LogLevel level) const;
    
    /**
     * @brief Destructor
     */
    ~Logger();
    
    // Delete copy and move constructors/assignments
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    std::string formatMessage(const char* fmt, ...);
    
    FILE* stream_;
    LogLevel level_;
    bool coloredOutput_;
    std::mutex mutex_;
    std::string prefix_;
};

// Global logger
extern std::shared_ptr<Logger> defaultLogger;

// Initialize library logging
void initializeLogging();

// Clean up library logging
void cleanupLogging();

// Convenience macros for logging
#define COIL_TRACE(logger, ...) \
    if (logger && logger->isLevelEnabled(coil::LogLevel::Trace)) \
        logger->log(coil::LogLevel::Trace, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_DEBUG(logger, ...) \
    if (logger && logger->isLevelEnabled(coil::LogLevel::Debug)) \
        logger->log(coil::LogLevel::Debug, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_INFO(logger, ...) \
    if (logger && logger->isLevelEnabled(coil::LogLevel::Info)) \
        logger->log(coil::LogLevel::Info, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_WARNING(logger, ...) \
    if (logger && logger->isLevelEnabled(coil::LogLevel::Warning)) \
        logger->log(coil::LogLevel::Warning, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_ERROR(logger, ...) \
    if (logger && logger->isLevelEnabled(coil::LogLevel::Error)) \
        logger->log(coil::LogLevel::Error, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_FATAL(logger, ...) \
    if (logger && logger->isLevelEnabled(coil::LogLevel::Fatal)) \
        logger->log(coil::LogLevel::Fatal, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Default logger convenience macros
#define COIL_DEFAULT_TRACE(...) COIL_TRACE(coil::defaultLogger, __VA_ARGS__)
#define COIL_DEFAULT_DEBUG(...) COIL_DEBUG(coil::defaultLogger, __VA_ARGS__)
#define COIL_DEFAULT_INFO(...) COIL_INFO(coil::defaultLogger, __VA_ARGS__)
#define COIL_DEFAULT_WARNING(...) COIL_WARNING(coil::defaultLogger, __VA_ARGS__)
#define COIL_DEFAULT_ERROR(...) COIL_ERROR(coil::defaultLogger, __VA_ARGS__)
#define COIL_DEFAULT_FATAL(...) COIL_FATAL(coil::defaultLogger, __VA_ARGS__)

} // namespace coil