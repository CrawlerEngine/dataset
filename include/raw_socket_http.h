#ifndef RAW_SOCKET_HTTP_H
#define RAW_SOCKET_HTTP_H

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "http_config.h"

struct RawHttpResponse {
    int status_code = 0;
    std::string body;
    std::string content_type;
    HTTPVersion http_version = HTTPVersion::UNKNOWN;
    std::string final_url;
    bool success = false;
    std::string error_message;
};

struct RawSocketRetryConfig {
    int max_retries = 2;
    int retry_backoff_ms = 200;
};

struct RawSocketHttpConfig {
    std::chrono::seconds timeout = std::chrono::seconds(30);
    RawSocketRetryConfig retry;
};

class CoroutineTask {
public:
    virtual ~CoroutineTask() = default;
    virtual bool step() = 0;
    virtual bool is_complete() const = 0;
};

class RoundRobinScheduler {
public:
    void add_task(const std::shared_ptr<CoroutineTask>& task);
    void run();

private:
    std::vector<std::shared_ptr<CoroutineTask>> tasks_;
};

class RawSocketHttpClient {
public:
    explicit RawSocketHttpClient(const RawSocketHttpConfig& config);
    RawHttpResponse fetch(const std::string& url,
                          const std::map<std::string, std::string>& headers);

private:
    RawSocketHttpConfig config_;
};

#endif // RAW_SOCKET_HTTP_H
