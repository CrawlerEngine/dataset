#include "raw_socket_http.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <map>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <unordered_map>
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

struct ResolvedAddress {
    sockaddr_storage addr {};
    socklen_t addr_len = 0;
    int family = AF_UNSPEC;
    int socktype = SOCK_STREAM;
    int protocol = 0;
    std::chrono::steady_clock::time_point expires_at;
};

std::unordered_map<std::string, ResolvedAddress> g_dns_cache;
std::mutex g_dns_mutex;
constexpr std::chrono::seconds kDnsCacheTtl(300);

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

bool resolve_host_cached(const std::string& host,
                         int port,
                         ResolvedAddress& out,
                         std::string& error) {
    const std::string key = host + ":" + std::to_string(port);
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(g_dns_mutex);
        auto it = g_dns_cache.find(key);
        if (it != g_dns_cache.end() && now < it->second.expires_at) {
            out = it->second;
            return true;
        }
    }

    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* addr_info = nullptr;
    const std::string port_str = std::to_string(port);
    int result = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &addr_info);
    if (result != 0) {
        error = gai_strerror(result);
        return false;
    }

    ResolvedAddress resolved;
    resolved.family = addr_info->ai_family;
    resolved.socktype = addr_info->ai_socktype;
    resolved.protocol = addr_info->ai_protocol;
    resolved.addr_len = static_cast<socklen_t>(addr_info->ai_addrlen);
    std::memcpy(&resolved.addr, addr_info->ai_addr, addr_info->ai_addrlen);
    resolved.expires_at = now + kDnsCacheTtl;
    freeaddrinfo(addr_info);

    {
        std::lock_guard<std::mutex> lock(g_dns_mutex);
        g_dns_cache[key] = resolved;
    }

    out = resolved;
    return true;
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

std::string trim(const std::string& value) {
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

struct ParsedHeaders {
    size_t header_end = std::string::npos;
    bool chunked = false;
    bool has_content_length = false;
    size_t content_length = 0;
    std::string location;
    std::string content_type;
};

ParsedHeaders parse_headers(const std::string& buffer) {
    ParsedHeaders headers;
    headers.header_end = buffer.find("\r\n\r\n");
    if (headers.header_end == std::string::npos) {
        return headers;
    }

    std::string header_block = buffer.substr(0, headers.header_end);
    std::istringstream header_stream(header_block);
    std::string status_line;
    std::getline(header_stream, status_line);

    std::string header_line;
    while (std::getline(header_stream, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        auto delimiter_pos = header_line.find(':');
        if (delimiter_pos == std::string::npos) {
            continue;
        }
        std::string key = to_lower(trim(header_line.substr(0, delimiter_pos)));
        std::string value = trim(header_line.substr(delimiter_pos + 1));
        if (key == "content-type") {
            headers.content_type = value;
        } else if (key == "content-length") {
            try {
                headers.has_content_length = true;
                headers.content_length = static_cast<size_t>(std::stoul(value));
            } catch (const std::exception&) {
                headers.has_content_length = false;
                headers.content_length = 0;
            }
        } else if (key == "transfer-encoding" && to_lower(value).find("chunked") != std::string::npos) {
            headers.chunked = true;
        } else if (key == "location") {
            headers.location = value;
        }
    }

    return headers;
}

bool decode_chunked_body(const std::string& input, std::string& output, bool& complete) {
    complete = false;
    size_t pos = 0;
    output.clear();
    while (true) {
        size_t line_end = input.find("\r\n", pos);
        if (line_end == std::string::npos) {
            return false;
        }
        std::string size_str = input.substr(pos, line_end - pos);
        size_t semicolon = size_str.find(';');
        if (semicolon != std::string::npos) {
            size_str = size_str.substr(0, semicolon);
        }
        size_t chunk_size = 0;
        std::istringstream size_stream(size_str);
        size_stream >> std::hex >> chunk_size;
        if (size_stream.fail()) {
            return false;
        }
        pos = line_end + 2;
        if (chunk_size == 0) {
            complete = true;
            return true;
        }
        if (pos + chunk_size + 2 > input.size()) {
            return false;
        }
        output.append(input, pos, chunk_size);
        pos += chunk_size + 2;
    }
}

std::string resolve_redirect(const ParsedUrl& base, const std::string& location) {
    if (location.empty()) {
        return "";
    }
    if (location.find("http://") == 0 || location.find("https://") == 0) {
        return location;
    }
    if (location.rfind("//", 0) == 0) {
        return base.scheme + ":" + location;
    }
    if (!location.empty() && location[0] == '/') {
        return base.scheme + "://" + base.host + location;
    }
    return base.scheme + "://" + base.host + "/" + location;
}

void set_socket_timeouts(int socket_fd, std::chrono::seconds timeout) {
    struct timeval tv {};
    tv.tv_sec = static_cast<long>(timeout.count());
    tv.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int connect_with_timeout(const ResolvedAddress& resolved,
                         std::chrono::seconds timeout,
                         std::string& error) {
    int socket_fd = socket(resolved.family, resolved.socktype, resolved.protocol);
    if (socket_fd < 0) {
        error = std::strerror(errno);
        return -1;
    }

    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        error = "failed to set non-blocking socket";
        close(socket_fd);
        return -1;
    }

    int connect_result = connect(socket_fd,
                                 reinterpret_cast<const sockaddr*>(&resolved.addr),
                                 resolved.addr_len);
    if (connect_result == 0) {
        fcntl(socket_fd, F_SETFL, flags);
        set_socket_timeouts(socket_fd, timeout);
        return socket_fd;
    }
    if (errno != EINPROGRESS) {
        error = std::strerror(errno);
        close(socket_fd);
        return -1;
    }

    struct pollfd pfd {};
    pfd.fd = socket_fd;
    pfd.events = POLLOUT;
    int poll_result = poll(&pfd, 1, static_cast<int>(timeout.count() * 1000));
    if (poll_result <= 0) {
        error = poll_result == 0 ? "connect timeout" : std::strerror(errno);
        close(socket_fd);
        return -1;
    }

    int socket_error = 0;
    socklen_t len = sizeof(socket_error);
    if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &len) != 0 || socket_error != 0) {
        error = socket_error == 0 ? "connect failed" : std::strerror(socket_error);
        close(socket_fd);
        return -1;
    }

    fcntl(socket_fd, F_SETFL, flags);
    set_socket_timeouts(socket_fd, timeout);
    return socket_fd;
}

RawHttpResponse parse_http_response(const std::string& buffer, const std::string& url) {
    RawHttpResponse response;
    ParsedHeaders headers = parse_headers(buffer);
    if (headers.header_end == std::string::npos) {
        response.error_message = "invalid HTTP response";
        return response;
    }

    std::string header_block = buffer.substr(0, headers.header_end);
    std::string body = buffer.substr(headers.header_end + 4);
    response.final_url = url;
    response.content_type = headers.content_type;
    response.location = headers.location;

    std::istringstream header_stream(header_block);
    std::string status_line;
    if (std::getline(header_stream, status_line)) {
        if (!status_line.empty() && status_line.back() == '\r') {
            status_line.pop_back();
        }
        response.http_version = parse_http_version(status_line);
        std::istringstream status_parser(status_line);
        std::string http_version;
        status_parser >> http_version >> response.status_code;
    }

    if (headers.chunked) {
        bool complete = false;
        std::string decoded;
        if (decode_chunked_body(body, decoded, complete) && complete) {
            response.body = decoded;
        } else {
            response.error_message = "incomplete chunked response";
            response.body = body;
        }
    } else if (headers.has_content_length) {
        response.body = body.substr(0, headers.content_length);
    } else {
        response.body = body;
    }

    response.success = response.status_code > 0;
    return response;
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
        if (!resolve_host_cached(parsed_.host, parsed_.port, resolved_, response_.error_message)) {
            return false;
        }

        socket_fd_ = socket(resolved_.family, resolved_.socktype, resolved_.protocol);
        if (socket_fd_ < 0) {
            response_.error_message = std::strerror(errno);
            return false;
        }

        int flags = fcntl(socket_fd_, F_GETFL, 0);
        if (flags < 0 || fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
            response_.error_message = "failed to set non-blocking socket";
            return false;
        }

        int connect_result = connect(socket_fd_,
                                     reinterpret_cast<const sockaddr*>(&resolved_.addr),
                                     resolved_.addr_len);
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
        if (!headers_parsed_) {
            ParsedHeaders headers = parse_headers(response_buffer_);
            if (headers.header_end != std::string::npos) {
                headers_parsed_ = true;
                header_end_pos_ = headers.header_end;
                content_length_ = headers.content_length;
                has_content_length_ = headers.has_content_length;
                chunked_ = headers.chunked;
            }
        }

        if (headers_parsed_) {
            std::string body = response_buffer_.substr(header_end_pos_ + 4);
            if (chunked_) {
                bool complete = false;
                std::string decoded;
                if (decode_chunked_body(body, decoded, complete) && complete) {
                    response_buffer_ = response_buffer_.substr(0, header_end_pos_ + 4) + decoded;
                    finalize_response();
                    complete_ = true;
                    return false;
                }
            } else if (has_content_length_) {
                if (body.size() >= content_length_) {
                    response_buffer_.resize(header_end_pos_ + 4 + content_length_);
                    finalize_response();
                    complete_ = true;
                    return false;
                }
            }
        }
        return true;
    }

    void build_request() {
        std::ostringstream request_stream;
        request_stream << "GET " << parsed_.path << " HTTP/1.1\r\n";
        request_stream << "Host: " << parsed_.host << "\r\n";
        request_stream << "Connection: keep-alive\r\n";
        request_stream << "User-Agent: DatasetCrawler/1.0\r\n";
        for (const auto& header : headers_) {
            request_stream << header.first << ": " << header.second << "\r\n";
        }
        request_stream << "\r\n";
        request_ = request_stream.str();
    }

    void finalize_response() {
        response_ = parse_http_response(response_buffer_, url_);
    }

    std::string url_;
    std::map<std::string, std::string> headers_;
    std::chrono::seconds timeout_;
    std::chrono::steady_clock::time_point start_time_;
    ParsedUrl parsed_;
    ResolvedAddress resolved_ {};
    int socket_fd_ = -1;
    State state_ = State::Init;
    bool complete_ = false;
    std::string request_;
    size_t request_offset_ = 0;
    std::string response_buffer_;
    RawHttpResponse response_;
    bool headers_parsed_ = false;
    size_t header_end_pos_ = 0;
    size_t content_length_ = 0;
    bool has_content_length_ = false;
    bool chunked_ = false;
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
    ParsedUrl parsed = parse_url(url);
    std::string current_url = url;
    int attempts = std::max(1, config_.retry.max_retries + 1);

    if (parsed.valid && parsed.scheme == "https") {
        int redirects_remaining = std::max(0, config_.max_redirects);
        for (int attempt = 0; attempt < attempts; ++attempt) {
            std::string error;
            parsed = parse_url(current_url);
            ResolvedAddress resolved;
            if (!resolve_host_cached(parsed.host, parsed.port, resolved, error)) {
                response.error_message = error;
                return response;
            }

            int socket_fd = connect_with_timeout(resolved, config_.timeout, error);
            if (socket_fd < 0) {
                response.error_message = error;
                continue;
            }

            SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
            if (!ctx) {
                response.error_message = "failed to create SSL context";
                close(socket_fd);
                return response;
            }

            SSL* ssl = SSL_new(ctx);
            if (!ssl) {
                response.error_message = "failed to create SSL session";
                SSL_CTX_free(ctx);
                close(socket_fd);
                return response;
            }

            SSL_set_fd(ssl, socket_fd);
            SSL_set_tlsext_host_name(ssl, parsed.host.c_str());
            if (SSL_connect(ssl) <= 0) {
                response.error_message = "TLS handshake failed";
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                close(socket_fd);
                continue;
            }

            std::ostringstream request_stream;
            request_stream << "GET " << parsed.path << " HTTP/1.1\r\n";
            request_stream << "Host: " << parsed.host << "\r\n";
            request_stream << "Connection: keep-alive\r\n";
            request_stream << "User-Agent: DatasetCrawler/1.0\r\n";
            for (const auto& header : headers) {
                request_stream << header.first << ": " << header.second << "\r\n";
            }
            request_stream << "\r\n";
            std::string request = request_stream.str();

            size_t total_written = 0;
            while (total_written < request.size()) {
                int written = SSL_write(ssl, request.data() + total_written,
                                        static_cast<int>(request.size() - total_written));
                if (written <= 0) {
                    response.error_message = "TLS write failed";
                    break;
                }
                total_written += static_cast<size_t>(written);
            }

            std::string response_buffer;
            if (total_written == request.size()) {
                char buffer[4096];
                int read_bytes = 0;
                while ((read_bytes = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
                    response_buffer.append(buffer, static_cast<size_t>(read_bytes));
                }
                if (!response_buffer.empty()) {
                    response = parse_http_response(response_buffer, current_url);
                } else if (response.error_message.empty()) {
                    response.error_message = "empty HTTPS response";
                }
            }

            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            close(socket_fd);

            if (response.success) {
                if (response.status_code == 301 || response.status_code == 302 ||
                    response.status_code == 303 || response.status_code == 307 ||
                    response.status_code == 308) {
                    std::string next_url = resolve_redirect(parsed, response.location);
                    if (!next_url.empty() && redirects_remaining > 0) {
                        current_url = next_url;
                        redirects_remaining--;
                        continue;
                    }
                }
                return response;
            }

            if (attempt + 1 < attempts) {
                int backoff = config_.retry.retry_backoff_ms * (attempt + 1);
                poll(nullptr, 0, backoff);
            }
        }

        return response;
    }

    int redirects_remaining = std::max(0, config_.max_redirects);
    for (int attempt = 0; attempt < attempts; ++attempt) {
        auto task = std::make_shared<HttpFetchCoroutine>(current_url, headers, config_.timeout);
        RoundRobinScheduler scheduler;
        scheduler.add_task(task);
        scheduler.run();

        response = task->response();
        if (response.success) {
            if (response.status_code == 301 || response.status_code == 302 ||
                response.status_code == 303 || response.status_code == 307 ||
                response.status_code == 308) {
                ParsedUrl current_parsed = parse_url(current_url);
                std::string next_url = resolve_redirect(current_parsed, response.location);
                if (!next_url.empty() && redirects_remaining > 0) {
                    current_url = next_url;
                    redirects_remaining--;
                    continue;
                }
            }
            return response;
        }

        if (attempt + 1 < attempts) {
            int backoff = config_.retry.retry_backoff_ms * (attempt + 1);
            poll(nullptr, 0, backoff);
        }
    }

    return response;
}
