#include "logger.h"
#include <iomanip>
#include <sstream>

Logger::Logger() : min_level_(LogLevel::INFO), use_colors_(true) {
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm* timeinfo = std::gmtime(&time_t_now);
    std::ostringstream oss;
    
    oss << std::put_time(timeinfo, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return oss.str();
}

std::string Logger::get_level_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO ";
        case LogLevel::WARN:
            return "WARN ";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string Logger::colorize(const std::string& text, LogLevel level) {
    if (!use_colors_) {
        return text;
    }

    std::string color;
    switch (level) {
        case LogLevel::DEBUG:
            color = COLOR_DEBUG;
            break;
        case LogLevel::INFO:
            color = COLOR_INFO;
            break;
        case LogLevel::WARN:
            color = COLOR_WARN;
            break;
        case LogLevel::ERROR:
            color = COLOR_ERROR;
            break;
        default:
            return text;
    }

    return color + text + COLOR_RESET;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level_) {
        return;
    }

    std::string timestamp = get_timestamp();
    std::string level_str = get_level_string(level);
    
    // Format: 2026-01-25T07:01:20.143Z INFO  <message>
    std::ostringstream oss;
    oss << timestamp << " " << colorize(level_str, level) << "  " << message;

    std::cout << oss.str() << std::endl;
}

void Logger::log(LogLevel level, const std::string& message, const std::string& context) {
    if (level < min_level_) {
        return;
    }

    std::string timestamp = get_timestamp();
    std::string level_str = get_level_string(level);

    // Format: 2026-01-25T07:01:20.143Z INFO  [context] <message>
    std::ostringstream oss;
    oss << timestamp << " " << colorize(level_str, level) << "  [" 
        << context << "] " << message;

    std::cout << oss.str() << std::endl;
}
