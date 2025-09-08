#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace VulkanGameEngine {

/**
 * Logger provides a comprehensive logging system with colored output and different log levels.
 * 
 * Features:
 * - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Colored console output for better readability
 * - Timestamps for each log message
 * - Thread-safe logging
 * - Configurable log level filtering
 * - Easy-to-use macros for different log levels
 */
class Logger {
public:
    /**
     * Log levels in order of severity
     */
    enum class Level {
        TRACE = 0,  // Detailed trace information
        DEBUG = 1,  // Debug information
        INFO = 2,   // General information
        WARN = 3,   // Warning messages
        ERROR = 4,  // Error messages
        FATAL = 5   // Fatal error messages
    };

    /**
     * ANSI color codes for console output
     */
    static const std::string COLOR_RESET;
    static const std::string COLOR_BOLD;
    static const std::string COLOR_DIM;
    static const std::string COLOR_RED;
    static const std::string COLOR_GREEN;
    static const std::string COLOR_YELLOW;
    static const std::string COLOR_BLUE;
    static const std::string COLOR_CYAN;
    static const std::string COLOR_WHITE;

    /**
     * Gets the singleton logger instance
     */
    static Logger& getInstance();

    /**
     * Sets the minimum log level to display
     */
    void setLogLevel(Level level);

    /**
     * Gets the current log level
     */
    Level getLogLevel() const { return m_logLevel; }

    /**
     * Enables or disables colored output
     */
    void setColorEnabled(bool enabled) { m_colorEnabled = enabled; }

    /**
     * Checks if colored output is enabled
     */
    bool isColorEnabled() const { return m_colorEnabled; }

    /**
     * Enables or disables timestamps
     */
    void setTimestampEnabled(bool enabled) { m_timestampEnabled = enabled; }

    /**
     * Checks if timestamps are enabled
     */
    bool isTimestampEnabled() const { return m_timestampEnabled; }

    /**
     * Logs a message at the specified level
     */
    void log(Level level, const std::string& message, const std::string& category = "");

    /**
     * Logs a trace message (most verbose)
     */
    void trace(const std::string& message, const std::string& category = "");

    /**
     * Logs a debug message
     */
    void debug(const std::string& message, const std::string& category = "");

    /**
     * Logs an info message
     */
    void info(const std::string& message, const std::string& category = "");

    /**
     * Logs a warning message
     */
    void warn(const std::string& message, const std::string& category = "");

    /**
     * Logs an error message
     */
    void error(const std::string& message, const std::string& category = "");

    /**
     * Logs a fatal error message
     */
    void fatal(const std::string& message, const std::string& category = "");

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Level m_logLevel;
    bool m_colorEnabled;
    bool m_timestampEnabled;

    /**
     * Gets the color code for a log level
     */
    std::string getLevelColor(Level level) const;

    /**
     * Gets the string representation of a log level
     */
    const char* getLevelString(Level level) const;

    /**
     * Gets the current timestamp as a string
     */
    std::string getCurrentTimestamp() const;

    /**
     * Formats and outputs the log message
     */
    void output(Level level, const std::string& message, const std::string& category) const;

    /**
     * Enables ANSI color codes on Windows console
     */
    void enableWindowsAnsiColors();


};

} // namespace VulkanGameEngine

// Convenient logging macros
#define LOG_TRACE(message, ...) VulkanGameEngine::Logger::getInstance().trace(message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) VulkanGameEngine::Logger::getInstance().debug(message, ##__VA_ARGS__)
#define LOG_INFO(message, ...)  VulkanGameEngine::Logger::getInstance().info(message, ##__VA_ARGS__)
#define LOG_WARN(message, ...)  VulkanGameEngine::Logger::getInstance().warn(message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) VulkanGameEngine::Logger::getInstance().error(message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) VulkanGameEngine::Logger::getInstance().fatal(message, ##__VA_ARGS__)

// Shorter aliases (avoid ERROR macro conflict with Windows)
#define LOG(message, ...)   LOG_INFO(message, ##__VA_ARGS__)
#define INFO(message, ...)  LOG_INFO(message, ##__VA_ARGS__)
#define WARN(message, ...)  LOG_WARN(message, ##__VA_ARGS__)
#define DEBUG(message, ...) LOG_DEBUG(message, ##__VA_ARGS__)
#define TRACE(message, ...) LOG_TRACE(message, ##__VA_ARGS__)

// Vulkan-specific logging macros
#define VK_LOG_INFO(message)    LOG_INFO(message, "Vulkan")
#define VK_LOG_ERROR(message)   LOG_ERROR(message, "Vulkan")
#define VK_LOG_WARN(message)    LOG_WARN(message, "Vulkan")
#define VK_LOG_DEBUG(message)   LOG_DEBUG(message, "Vulkan")

// Object lifecycle logging
#define LOG_OBJECT_CREATED(type, name)    LOG_DEBUG("Created " + std::string(type) + ": " + std::string(name), "Object")
#define LOG_OBJECT_DESTROYED(type, name)  LOG_DEBUG("Destroyed " + std::string(type) + ": " + std::string(name), "Object")

// Performance logging
#define LOG_PERF_START(operation) auto _perf_start_##operation = std::chrono::high_resolution_clock::now()
#define LOG_PERF_END(operation) do { \
    auto _perf_end = std::chrono::high_resolution_clock::now(); \
    auto _perf_duration = std::chrono::duration<float, std::milli>(_perf_end - _perf_start_##operation); \
    LOG_DEBUG(#operation " took " + std::to_string(_perf_duration.count()) + "ms", "Performance"); \
} while(0)