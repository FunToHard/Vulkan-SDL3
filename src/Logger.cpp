#include "../headers/Logger.h"
#include <mutex>
#include <iostream>

// Include platform-specific headers after our definitions
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

// Windows API declarations for ANSI color support (must be at global scope)
#ifdef _WIN32
extern "C" {
    __declspec(dllimport) void* __stdcall GetStdHandle(unsigned long nStdHandle);
    __declspec(dllimport) int __stdcall GetConsoleMode(void* hConsoleHandle, unsigned long* lpMode);
    __declspec(dllimport) int __stdcall SetConsoleMode(void* hConsoleHandle, unsigned long dwMode);
}
#endif

namespace VulkanGameEngine {

// Define color constants
const std::string Logger::COLOR_RESET = "\033[0m";
const std::string Logger::COLOR_BOLD = "\033[1m";
const std::string Logger::COLOR_DIM = "\033[2m";
const std::string Logger::COLOR_RED = "\033[31m";
const std::string Logger::COLOR_GREEN = "\033[32m";
const std::string Logger::COLOR_YELLOW = "\033[33m";
const std::string Logger::COLOR_BLUE = "\033[34m";
const std::string Logger::COLOR_CYAN = "\033[36m";
const std::string Logger::COLOR_WHITE = "\033[37m";

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : m_logLevel(Level::INFO)
    , m_colorEnabled(true)
    , m_timestampEnabled(true) {
    
    // Detect if we're outputting to a terminal that supports colors
#ifdef _WIN32
    // Check if stdout is a terminal
    m_colorEnabled = isatty(fileno(stdout));
    
    // Enable ANSI color codes on Windows 10+
    if (m_colorEnabled) {
        enableWindowsAnsiColors();
    }
#else
    // On Unix-like systems, check if stdout is a terminal
    m_colorEnabled = isatty(fileno(stdout));
#endif
}

void Logger::setLogLevel(Level level) {
    m_logLevel = level;
}

void Logger::log(Level level, const std::string& message, const std::string& category) {
    if (level < m_logLevel) {
        return; // Skip messages below the current log level
    }
    
    output(level, message, category);
}

void Logger::trace(const std::string& message, const std::string& category) {
    log(Level::TRACE, message, category);
}

void Logger::debug(const std::string& message, const std::string& category) {
    log(Level::DEBUG, message, category);
}

void Logger::info(const std::string& message, const std::string& category) {
    log(Level::INFO, message, category);
}

void Logger::warn(const std::string& message, const std::string& category) {
    log(Level::WARN, message, category);
}

void Logger::error(const std::string& message, const std::string& category) {
    log(Level::ERROR, message, category);
}

void Logger::fatal(const std::string& message, const std::string& category) {
    log(Level::FATAL, message, category);
}

std::string Logger::getLevelColor(Level level) const {
    if (!m_colorEnabled) {
        return "";
    }
    
    switch (level) {
        case Level::TRACE:
            return COLOR_DIM;
        case Level::DEBUG:
            return COLOR_CYAN;
        case Level::INFO:
            return COLOR_GREEN;
        case Level::WARN:
            return COLOR_YELLOW;
        case Level::ERROR:
            return COLOR_RED;
        case Level::FATAL:
            return COLOR_BOLD + COLOR_RED;
        default:
            return COLOR_RESET;
    }
}

const char* Logger::getLevelString(Level level) const {
    switch (level) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO ";
        case Level::WARN:  return "WARN ";
        case Level::ERROR: return "ERROR";
        case Level::FATAL: return "FATAL";
        default:           return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

void Logger::output(Level level, const std::string& message, const std::string& category) const {
    static std::mutex logMutex; // Thread-safe logging
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::ostream& stream = (level >= Level::ERROR) ? std::cerr : std::cout;
    
    std::string levelColor = getLevelColor(level);
    
    // Start with color if enabled
    if (m_colorEnabled) {
        stream << levelColor;
    }
    
    // Add timestamp if enabled
    if (m_timestampEnabled) {
        if (m_colorEnabled) {
            stream << COLOR_DIM << "[" << getCurrentTimestamp() << "] " << COLOR_RESET;
            stream << levelColor; // Restore level color after timestamp
        } else {
            stream << "[" << getCurrentTimestamp() << "] ";
        }
    }
    
    // Add log level
    stream << "[" << getLevelString(level) << "]";
    
    // Add category if provided
    if (!category.empty()) {
        if (m_colorEnabled) {
            stream << COLOR_RESET << COLOR_DIM << "[" << category << "]" << COLOR_RESET;
            stream << levelColor; // Restore level color after category
        } else {
            stream << "[" << category << "]";
        }
    }
    
    // Add the message
    stream << " " << message;
    
    // Reset color if enabled
    if (m_colorEnabled) {
        stream << COLOR_RESET;
    }
    
    stream << std::endl;
    
    // Flush immediately for error and fatal messages
    if (level >= Level::ERROR) {
        stream.flush();
    }
}

void Logger::enableWindowsAnsiColors() {
#ifdef _WIN32
    // Use GetStdHandle and SetConsoleMode to enable ANSI colors
    void* hOut = GetStdHandle((unsigned long)-11); // STD_OUTPUT_HANDLE = -11
    if (hOut != (void*)-1) { // INVALID_HANDLE_VALUE = -1
        unsigned long dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

} // namespace VulkanGameEngine