#include "raw_socket_http.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string path;
    int port = 80;
    bool valid = false;
};

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl parsed;
    const std::string scheme_delim = "://";
    auto scheme_pos = url.find(scheme_delim);
    if (scheme_pos == std::string::npos) {
        return parsed;
    }

    parsed.scheme = url.substr(0, scheme_pos);
    std::string rest = url.substr(scheme_pos + scheme_delim.size());
    auto path_pos = rest.find('/');
    std::string host_port = path_pos == std::string::npos ? rest : rest.substr(0, path_pos);
    parsed.path = path_pos == std::string::npos ? "/" : rest.substr(path_pos);

    auto port_pos = host_port.find(':');
    if (port_pos != std::string::npos) {
        parsed.host = host_port.substr(0, port_pos);
        parsed.port = std::stoi(host_port.substr(port_pos + 1));
    } else {
        parsed.host = host_port;
        parsed.port = parsed.scheme == "https" ? 443 : 80;
    }

    parsed.valid = !parsed.scheme.empty() && !parsed.host.empty();
    return parsed;
}

HTTPVersion parse_http_version(const std::string& status_line) {
    if (status_line.find("HTTP/1.0") == 0) {
        return HTTPVersion::HTTP_1_0;
    }
    if (status_line.find("HTTP/1.1") == 0) {
        return HTTPVersion::HTTP_1_1;
    }
    if (status_line.find("HTTP/2") == 0) {
        return HTTPVersion::HTTP_2_0;
    }
    return HTTPVersion::UNKNOWN;
}

std::string to_lower(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return input;
}

class HttpFetchCoroutine : public CoroutineTask {
public:
    HttpFetchCoroutine(const std::string& url,
                       const std::map<std::string, std::string>& headers,
                       std::chrono::seconds timeout)
        : url_(url),
          headers_(headers),
          timeout_(timeout),
          start_time_(std::chrono::steady_clock::now()) {
        parsed_ = parse_url(url);
    }

    ~HttpFetchCoroutine() override {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
        }
        if (addr_info_) {
            freeaddrinfo(addr_info_);
        }
    }

    bool step() override {
        if (complete_) {
            return false;
        }

        if (!parsed_.valid || parsed_.scheme != "http") {
            response_.error_message = "raw socket fetch supports http:// only";
            complete_ = true;
            return false;
        }

        if (std::chrono::steady_clock::now() - start_time_ > timeout_) {
            response_.error_message = "raw socket fetch timeout";
            complete_ = true;
            return false;
        }

        switch (state_) {
            case State::Init:
                if (!init_socket()) {
                    complete_ = true;
                    return false;
                }
                return true;
            case State::Connecting:
                return handle_connecting();
            case State::Sending:
                return handle_sending();
            case State::Reading:
                return handle_reading();
        }

        return false;
    }

    bool is_complete() const override {
        return complete_;
    }

    const RawHttpResponse& response() const {
        return response_;
    }

private:
    enum class State {
        Init,
        Connecting,
        Sending,
        Reading
    };

    bool init_socket() {
        struct addrinfo hints {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        const std::string port_str = std::to_string(parsed_.port);
        int result = getaddrinfo(parsed_.host.c_str(), port_str.c_str(), &hints, &addr_info_);
        if (result != 0) {
            response_.error_message = gai_strerror(result);
            return false;
        }

        socket_fd_ = socket(addr_info_->ai_family, addr_info_->ai_socktype, addr_info_->ai_protocol);
        if (socket_fd_ < 0) {
            response_.error_message = std::strerror(errno);
            return false;
        }

        int flags = fcntl(socket_fd_, F_GETFL, 0);
        if (flags < 0 || fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
            response_.error_message = "failed to set non-blocking socket";
            return false;
        }

        int connect_result = connect(socket_fd_, addr_info_->ai_addr, addr_info_->ai_addrlen);
        if (connect_result == 0) {
            state_ = State::Sending;
        } else if (errno == EINPROGRESS) {
            state_ = State::Connecting;
        } else {
            response_.error_message = std::strerror(errno);
            return false;
        }

        build_request();
        return true;
    }

    bool handle_connecting() {
        struct pollfd pfd {};
        pfd.fd = socket_fd_;
        pfd.events = POLLOUT;
        int poll_result = poll(&pfd, 1, 0);
        if (poll_result == 0) {
            return true;
        }
        if (poll_result < 0) {
            response_.error_message = std::strerror(errno);
            complete_ = true;
            return false;
        }

        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0) {
            response_.error_message = error == 0 ? "connect failed" : std::strerror(error);
            complete_ = true;
            return false;
        }

        state_ = State::Sending;
        return true;
    }

    bool handle_sending() {
        if (request_offset_ >= request_.size()) {
            state_ = State::Reading;
            return true;
        }

        ssize_t sent = send(socket_fd_, request_.data() + request_offset_,
                            request_.size() - request_offset_, 0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            }
            response_.error_message = std::strerror(errno);
            complete_ = true;
            return false;
        }

        request_offset_ += static_cast<size_t>(sent);
        return true;
    }

    bool handle_reading() {
        char buffer[4096];
        ssize_t received = recv(socket_fd_, buffer, sizeof(buffer), 0);
        if (received == 0) {
            finalize_response();
            complete_ = true;
            return false;
        }
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            }
            response_.error_message = std::strerror(errno);
            complete_ = true;
            return false;
        }

        response_buffer_.append(buffer, static_cast<size_t>(received));
        return true;
    }

    void build_request() {
        std::ostringstream request_stream;
        request_stream << "GET " << parsed_.path << " HTTP/1.1\r\n";
        request_stream << "Host: " << parsed_.host << "\r\n";
        request_stream << "Connection: close\r\n";
        request_stream << "User-Agent: DatasetCrawler/1.0\r\n";
        for (const auto& header : headers_) {
            request_stream << header.first << ": " << header.second << "\r\n";
        }
        request_stream << "\r\n";
        request_ = request_stream.str();
    }

    void finalize_response() {
        auto header_end = response_buffer_.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            response_.error_message = "invalid HTTP response";
            return;
        }

        std::string header_block = response_buffer_.substr(0, header_end);
        response_.body = response_buffer_.substr(header_end + 4);
        response_.final_url = url_;

        std::istringstream header_stream(header_block);
        std::string status_line;
        if (std::getline(header_stream, status_line)) {
            if (!status_line.empty() && status_line.back() == '\r') {
                status_line.pop_back();
            }
            response_.http_version = parse_http_version(status_line);
            std::istringstream status_parser(status_line);
            std::string http_version;
            status_parser >> http_version >> response_.status_code;
        }

        std::string header_line;
        while (std::getline(header_stream, header_line)) {
            if (!header_line.empty() && header_line.back() == '\r') {
                header_line.pop_back();
            }
            auto delimiter_pos = header_line.find(':');
            if (delimiter_pos == std::string::npos) {
                continue;
            }
            std::string key = to_lower(header_line.substr(0, delimiter_pos));
            std::string value = header_line.substr(delimiter_pos + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            if (key == "content-type") {
                response_.content_type = value;
            }
        }

        response_.success = response_.status_code > 0;
    }

    std::string url_;
    std::map<std::string, std::string> headers_;
    std::chrono::seconds timeout_;
    std::chrono::steady_clock::time_point start_time_;
    ParsedUrl parsed_;
    struct addrinfo* addr_info_ = nullptr;
    int socket_fd_ = -1;
    State state_ = State::Init;
    bool complete_ = false;
    std::string request_;
    size_t request_offset_ = 0;
    std::string response_buffer_;
    RawHttpResponse response_;
};

} // namespace

void RoundRobinScheduler::add_task(const std::shared_ptr<CoroutineTask>& task) {
    tasks_.push_back(task);
}

void RoundRobinScheduler::run() {
    while (!tasks_.empty()) {
        for (size_t index = 0; index < tasks_.size();) {
            auto& task = tasks_[index];
            bool should_continue = task->step();
            if (task->is_complete() || !should_continue) {
                tasks_.erase(tasks_.begin() + static_cast<long>(index));
                continue;
            }
            ++index;
        }
        if (!tasks_.empty()) {
            poll(nullptr, 0, 5);
        }
    }
}

RawSocketHttpClient::RawSocketHttpClient(const RawSocketHttpConfig& config)
    : config_(config) {}

RawHttpResponse RawSocketHttpClient::fetch(const std::string& url,
                                           const std::map<std::string, std::string>& headers) {
    RawHttpResponse response;
    int attempts = std::max(1, config_.retry.max_retries + 1);
    for (int attempt = 0; attempt < attempts; ++attempt) {
        auto task = std::make_shared<HttpFetchCoroutine>(url, headers, config_.timeout);
        RoundRobinScheduler scheduler;
        scheduler.add_task(task);
        scheduler.run();

        response = task->response();
        if (response.success) {
            return response;
        }

        if (attempt + 1 < attempts) {
            int backoff = config_.retry.retry_backoff_ms * (attempt + 1);
            poll(nullptr, 0, backoff);
        }
    }

    return response;
}
