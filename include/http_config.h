#ifndef HTTP_CONFIG_H
#define HTTP_CONFIG_H

#include <string>
#include <curl/curl.h>

/**
 * HTTP/SSL Configuration and utilities
 * Supports HTTP/1.0, HTTP/1.1, and HTTP/2
 * Uses BoringSSL for TLS operations
 */

struct HTTPConfig {
    bool enable_http2 = true;      // Enable HTTP/2 (falls back to HTTP/1.1)
    bool enable_http_keep_alive = true;
    bool verify_ssl_cert = false;  // Crawler doesn't verify certs by default
    bool verify_ssl_host = false;
    int tcp_keepalive_idle = 120;  // Seconds
    int tcp_keepalive_interval = 60; // Seconds
    bool use_raw_sockets = true;   // Use raw-socket HTTP/1.1 fetch for http://
    int max_retries = 2;           // Auto-retries for fetch failures
    int retry_backoff_ms = 200;    // Base backoff between retries
    bool enable_adaptive_delay = true; // Enable adaptive delay between requests
    int min_delay_ms = 50;         // Minimum adaptive delay
    int max_delay_ms = 2000;       // Maximum adaptive delay
    int base_delay_ms = 150;       // Baseline delay for low-latency responses
    double latency_ema_alpha = 0.2; // EMA factor for latency smoothing
    int failure_backoff_ms = 250;  // Extra backoff per failure streak
    int jitter_pct = 10;           // Random jitter percentage applied to delay
};

enum class HTTPVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0,
    UNKNOWN
};

/**
 * Get HTTP version string representation
 */
inline std::string get_http_version_string(HTTPVersion version) {
    switch (version) {
        case HTTPVersion::HTTP_1_0:
            return "HTTP/1.0";
        case HTTPVersion::HTTP_1_1:
            return "HTTP/1.1";
        case HTTPVersion::HTTP_2_0:
            return "HTTP/2";
        default:
            return "HTTP/?.?";
    }
}

/**
 * Convert CURL HTTP version to our enum
 */
inline HTTPVersion curl_http_version_to_enum(long curl_version) {
    switch (curl_version) {
        case CURL_HTTP_VERSION_1_0:
            return HTTPVersion::HTTP_1_0;
        case CURL_HTTP_VERSION_1_1:
            return HTTPVersion::HTTP_1_1;
        case CURL_HTTP_VERSION_2_0:
            return HTTPVersion::HTTP_2_0;
        default:
            return HTTPVersion::UNKNOWN;
    }
}

#endif // HTTP_CONFIG_H
