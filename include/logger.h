#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <memory>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) {
        min_level_ = level;
    }

    void set_color_output(bool enable) {
        use_colors_ = enable;
    }

    // Core logging methods
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const std::string& message, const std::string& context);

    // Convenience methods
    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void debug(const std::string& msg, const std::string& ctx) { log(LogLevel::DEBUG, msg, ctx); }

    void info(const std::string& msg) { log(LogLevel::INFO, msg); }
    void info(const std::string& msg, const std::string& ctx) { log(LogLevel::INFO, msg, ctx); }

    void warn(const std::string& msg) { log(LogLevel::WARN, msg); }
    void warn(const std::string& msg, const std::string& ctx) { log(LogLevel::WARN, msg, ctx); }

    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }
    void error(const std::string& msg, const std::string& ctx) { log(LogLevel::ERROR, msg, ctx); }

    // Templated log method for easy formatting
    template<typename T>
    Logger& operator<<(const T& value) {
        buffer_ << value;
        return *this;
    }

    // For stream-like usage
    void flush(LogLevel level) {
        std::string msg = buffer_.str();
        if (!msg.empty()) {
            log(level, msg);
            buffer_.str("");
            buffer_.clear();
        }
    }

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string get_timestamp();
    std::string get_level_string(LogLevel level);
    std::string colorize(const std::string& text, LogLevel level);

    LogLevel min_level_;
    bool use_colors_;
    std::stringstream buffer_;

    // ANSI color codes
    static constexpr const char* COLOR_RESET = "\033[0m";
    static constexpr const char* COLOR_DEBUG = "\033[36m";    // Cyan
    static constexpr const char* COLOR_INFO = "\033[32m";     // Green
    static constexpr const char* COLOR_WARN = "\033[33m";     // Yellow
    static constexpr const char* COLOR_ERROR = "\033[31m";    // Red
    static constexpr const char* COLOR_DIM = "\033[2m";       // Dim (for timestamp)
};

// Global convenience functions
inline void log_debug(const std::string& msg) {
    Logger::instance().debug(msg);
}

inline void log_info(const std::string& msg) {
    Logger::instance().info(msg);
}

inline void log_warn(const std::string& msg) {
    Logger::instance().warn(msg);
}

inline void log_error(const std::string& msg) {
    Logger::instance().error(msg);
}

#endif // LOGGER_H
