#pragma once

#include <string>
#include <cstdio>
#include <functional>
#include <chrono>

namespace coil {

/**
 * @brief Log levels
 */
enum class LogLevel {
    Trace = 0,  ///< Trace level (most verbose)
    Debug,      ///< Debug level
    Info,       ///< Info level
    Warning,    ///< Warning level
    Error,      ///< Error level
    Fatal,      ///< Fatal level
    None        ///< No logging
};

// ANSI color codes - placed at namespace level to avoid recreation
namespace detail {
    static const char* const LOG_LEVEL_COLORS[] = {
        "\x1b[90m",  // TRACE: Bright Black
        "\x1b[36m",  // DEBUG: Cyan
        "\x1b[32m",  // INFO: Green
        "\x1b[33m",  // WARNING: Yellow
        "\x1b[31m",  // ERROR: Red
        "\x1b[35m",  // FATAL: Magenta
        ""           // NONE: No color
    };

    static const char* const LOG_LEVEL_NAMES[] = {
        "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"
    };

    static const char* const RESET_COLOR = "\x1b[0m";
}

/**
 * @brief Optimized logger class without thread safety or heap allocations
 */
class Logger {
public:
    /**
     * @brief Construct a logger with a specific stream
     * 
     * @param prefix Logger name/prefix
     * @param stream Output stream (stdout, file, etc.)
     * @param level Initial log level
     * @param useColors Whether to use ANSI colors for output
     */
    Logger(const std::string& prefix, FILE* stream, LogLevel level, bool useColors = true);
    
    /**
     * @brief Set the log level
     * 
     * @param level New log level
     */
    inline void setLevel(LogLevel level) { level_ = level; }
    
    /**
     * @brief Enable or disable colored output
     * 
     * @param enabled Whether to use ANSI colors
     */
    inline void setColoredOutput(bool enabled) { coloredOutput_ = enabled; }
    
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
    inline LogLevel getLevel() const { return level_; }
    
    /**
     * @brief Check if a level is enabled
     * 
     * @param level Level to check
     * @return true if enabled
     */
    inline bool isLevelEnabled(LogLevel level) const { return level >= level_; }
    
    /**
     * @brief Destructor
     */
    ~Logger() = default;

private:
    FILE* stream_;         // Owned externally
    LogLevel level_;
    bool coloredOutput_;
    std::string prefix_;
};

// Implementation of the template function
template<typename... Args>
void Logger::log(LogLevel level, const char* file, int line, const char* func, const char* fmt, Args&&... args) {
    if (level < level_) return;
    
    char timestamp[32];
    std::time_t now = std::time(nullptr);
    std::tm* tm_info = std::localtime(&now);
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Print timestamp and level
    if (coloredOutput_) {
        fprintf(stream_, "%s [%s%s%s] [%s] ", 
                timestamp, 
                detail::LOG_LEVEL_COLORS[static_cast<int>(level)], 
                detail::LOG_LEVEL_NAMES[static_cast<int>(level)], 
                detail::RESET_COLOR,
                prefix_.c_str());
    } else {
        fprintf(stream_, "%s [%s] [%s] ", 
                timestamp, 
                detail::LOG_LEVEL_NAMES[static_cast<int>(level)],
                prefix_.c_str());
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

// Convenience macros for logging - compile out trace and debug in release mode
#ifdef NDEBUG
    #define COIL_TRACE(logger, ...) ((void)0)
    #define COIL_DEBUG(logger, ...) ((void)0)
#else
    #define COIL_TRACE(logger, ...) \
        if ((logger).isLevelEnabled(coil::LogLevel::Trace)) \
            (logger).log(coil::LogLevel::Trace, __FILE__, __LINE__, __func__, __VA_ARGS__)

    #define COIL_DEBUG(logger, ...) \
        if ((logger).isLevelEnabled(coil::LogLevel::Debug)) \
            (logger).log(coil::LogLevel::Debug, __FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

#define COIL_INFO(logger, ...) \
    if ((logger).isLevelEnabled(coil::LogLevel::Info)) \
        (logger).log(coil::LogLevel::Info, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_WARNING(logger, ...) \
    if ((logger).isLevelEnabled(coil::LogLevel::Warning)) \
        (logger).log(coil::LogLevel::Warning, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_ERROR(logger, ...) \
    if ((logger).isLevelEnabled(coil::LogLevel::Error)) \
        (logger).log(coil::LogLevel::Error, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_FATAL(logger, ...) \
    if ((logger).isLevelEnabled(coil::LogLevel::Fatal)) \
        (logger).log(coil::LogLevel::Fatal, __FILE__, __LINE__, __func__, __VA_ARGS__)

} // namespace coil