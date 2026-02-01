# Structured Logging Guide

## Overview

The Dataset Crawler now includes a professional structured logging system with:

- **ISO 8601 Timestamps**: `2026-01-25T08:07:46.072Z`
- **Log Levels**: INFO, WARN, ERROR, DEBUG
- **Color-Coded Output**: Different colors for different log levels
- **Context Support**: Add context labels to logs for better tracking

## Log Format

```
2026-01-25T08:07:46.072Z INFO   Message text here
```

Format breakdown:
- **Timestamp**: `2026-01-25T08:07:46.072Z` (UTC, millisecond precision)
- **Log Level**: `INFO `, `WARN `, `ERROR`, `DEBUG` (color-coded)
- **Message**: The actual log message

## Log Levels

### INFO (Green) - ℹ️
General operational information about the crawler's progress.

```cpp
log_info("Crawling will be started using 110 start URLs and 0 sitemap URLs");
log_info("Successfully wrote 4 records to dataset.json");
```

### WARN (Yellow) - ⚠️
Warning messages about potentially problematic conditions that don't stop execution.

```cpp
log_warn("Failed to fetch robots.txt for request https://example.com");
log_warn("Received blocked status code: 429", "RateLimiter");
```

### ERROR (Red) - ❌
Error messages about serious problems that may stop execution.

```cpp
log_error("Connection timeout after 30 seconds", "WebCrawler");
log_error("Failed to write output file: /path/to/file.json");
```

### DEBUG (Cyan) - 🔍
Detailed diagnostic information for troubleshooting.

```cpp
log_debug("Cache hit for domain: example.com");
log_debug("robots.txt parsed successfully", "Parser");
```

## Usage

### Basic Usage

```cpp
#include "logger.h"

int main() {
    // Initialize logger
    Logger& log = Logger::instance();
    log.set_color_output(true);
    log.set_level(LogLevel::INFO);

    // Log messages
    log_info("Starting crawler");
    log_warn("Potential issue detected");
    log_error("Critical error occurred");
    
    return 0;
}
```

### With Context

```cpp
// Add context labels to identify where logs come from
log.info("Starting the crawler.", "Main");
log.warn("Failed to fetch robots.txt", "RobotParser");
log.error("Network error", "HTTPClient");
```

### Using Logger Instance Methods

```cpp
Logger& log = Logger::instance();

// Using convenience methods
log.info("Info message");
log.warn("Warning message");
log.error("Error message");
log.debug("Debug message");

// Using methods with context
log.info("Message", "ContextLabel");
log.warn("Warning", "Parser");
```

### Global Functions

```cpp
#include "logger.h"

// Global convenience functions
log_info("Message");
log_warn("Warning");
log_error("Error");
log_debug("Debug");
```

## Configuration

### Set Log Level

Control which log levels are displayed:

```cpp
Logger::instance().set_level(LogLevel::INFO);  // Shows INFO, WARN, ERROR
Logger::instance().set_level(LogLevel::DEBUG); // Shows all levels
Logger::instance().set_level(LogLevel::ERROR); // Shows only ERROR
```

### Enable/Disable Colors

```cpp
Logger::instance().set_color_output(true);   // Enable colors (default)
Logger::instance().set_color_output(false);  // Disable colors for CI/CD
```

## Typical Crawler Log Output

```
2026-01-25T08:07:46.072Z INFO   === Dataset Crawler for AI ===
2026-01-25T08:07:46.072Z INFO   C++ Web Crawler with Ethical Crawling Support
2026-01-25T08:07:46.073Z INFO   Crawling will be started using 4 start URLs and 0 sitemap URLs
2026-01-25T08:07:46.073Z INFO   Starting the crawler.
2026-01-25T08:07:46.122Z INFO   https://example.com [200]
2026-01-25T08:07:46.192Z INFO   https://www.wikipedia.org/wiki/Machine_learning [200]
2026-01-25T08:07:46.292Z INFO   Crawling completed. Fetched: 4 records, Blocked by robots.txt: 0, Blocked by noindex: 0
2026-01-25T08:07:46.293Z INFO   Successfully wrote 4 records to dataset.json
2026-01-25T08:07:46.293Z INFO   Successfully wrote 4 records to dataset.csv
```

## Error Scenarios

### robots.txt Fetch Failed

```
2026-01-25T08:01:21.764Z WARN  Failed to fetch robots.txt for request https://nlab-pages.org/nlab/show/HomePage
```

### Network Error

```
2026-01-25T08:01:28.428Z WARN  Reclaiming failed request back to the list or queue. page.goto: NS_ERROR_PROXY_CONNECTION_REFUSED
```

### Rate Limiting

```
2026-01-25T08:01:36.337Z WARN  Reclaiming failed request back to the list or queue. Detected a session error, rotating session...
2026-01-25T08:01:36.338Z WARN  Received blocked status code: 403
```

## Integration with CI/CD

For CI/CD pipelines, disable colors:

```cpp
int main() {
    // Check if running in CI environment
    bool is_ci = std::getenv("CI") != nullptr;
    
    Logger::instance().set_color_output(!is_ci);
    Logger::instance().set_level(LogLevel::INFO);
    
    // ... rest of code
}
```

## Examples

See the examples folder for:
- `logging_demo.cpp` - Basic logging demonstration
- `advanced_logs.cpp` - Advanced scenarios with multiple log types
- `ethical_crawler.cpp` - Real crawler with integrated logging

## Implementation Details

The logger is implemented as a singleton:

```cpp
// Always use the same logger instance throughout the application
Logger& log1 = Logger::instance();
Logger& log2 = Logger::instance();
// log1 and log2 refer to the same object
```

The logging system is thread-safe for reading, but log level changes should be made during initialization before multiple threads access the logger.

## Performance Notes

- Logging has minimal performance impact
- Timestamps use system clock for high precision
- Color codes only added when explicitly enabled
- No dynamic memory allocation in core logging path

## Compatibility

- **C++ Standard**: C++17
- **Platforms**: Linux, macOS, Windows (with ANSI support)
- **Colors**: ANSI color codes (widely supported in modern terminals)

To disable colors in environments without ANSI support, call:
```cpp
Logger::instance().set_color_output(false);
```
