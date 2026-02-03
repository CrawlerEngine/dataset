#include "crawler.h"
#include "dataset_writer.h"
#include "logger.h"
#include "config_loader.h"
#include "http_config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

std::atomic<bool>* g_stop_flag = nullptr;

void handle_signal(int /*signal*/) {
    if (g_stop_flag) {
        g_stop_flag->store(true);
    }
}

std::string url_decode(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size()) {
            std::string hex = input.substr(i + 1, 2);
            char* end = nullptr;
            long value = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                output.push_back(static_cast<char>(value));
                i += 2;
                continue;
            }
        } else if (input[i] == '+') {
            output.push_back(' ');
            continue;
        }
        output.push_back(input[i]);
    }
    return output;
}

bool extract_url_from_request(const std::string& request, std::string& url_out) {
    std::istringstream request_stream(request);
    std::string request_line;
    if (!std::getline(request_stream, request_line)) {
        return false;
    }
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }
    std::istringstream line_stream(request_line);
    std::string method;
    std::string target;
    line_stream >> method >> target;
    if (method.empty() || target.empty()) {
        return false;
    }

    if (target.find("/enqueue") == 0) {
        auto query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query = target.substr(query_pos + 1);
            auto url_pos = query.find("url=");
            if (url_pos != std::string::npos) {
                std::string value = query.substr(url_pos + 4);
                url_out = url_decode(value);
                return !url_out.empty();
            }
        }
    }

    auto body_pos = request.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        std::string body = request.substr(body_pos + 4);
        auto url_pos = body.find("url=");
        if (url_pos != std::string::npos) {
            std::string value = body.substr(url_pos + 4);
            url_out = url_decode(value);
            return !url_out.empty();
        }
        auto json_pos = body.find("\"url\"");
        if (json_pos != std::string::npos) {
            auto quote_pos = body.find('"', json_pos + 4);
            if (quote_pos != std::string::npos) {
                auto end_quote = body.find('"', quote_pos + 1);
                if (end_quote != std::string::npos) {
                    url_out = body.substr(quote_pos + 1, end_quote - quote_pos - 1);
                    return !url_out.empty();
                }
            }
        }
        if (body.find("http://") == 0 || body.find("https://") == 0) {
            url_out = body;
            return true;
        }
    }
    return false;
}

void run_api_server(WebCrawler* crawler,
                    const CrawlerConfig& config,
                    std::atomic<bool>* stop_flag) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_error("Failed to create API server socket");
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(config.api_port));
    if (inet_pton(AF_INET, config.api_bind_address.c_str(), &addr.sin_addr) <= 0) {
        log_error("Invalid API bind address: " + config.api_bind_address);
        close(server_fd);
        return;
    }

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        log_error("Failed to bind API server socket");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        log_error("Failed to listen on API server socket");
        close(server_fd);
        return;
    }

    std::ostringstream msg;
    msg << "API server listening on " << config.api_bind_address << ":" << config.api_port
        << " (use /enqueue?url=...)";
    log_info(msg.str());

    while (!stop_flag->load()) {
        struct pollfd pfd {};
        pfd.fd = server_fd;
        pfd.events = POLLIN;
        int poll_result = poll(&pfd, 1, 500);
        if (poll_result <= 0) {
            continue;
        }

        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            continue;
        }

        char buffer[8192];
        ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            close(client_fd);
            continue;
        }
        buffer[received] = '\0';
        std::string request(buffer);

        std::string url;
        bool ok = extract_url_from_request(request, url);
        if (ok) {
            bool enqueued = crawler->enqueue_url(url);
            if (enqueued) {
                log_info("API enqueue: " + url);
            } else {
                log_warn("API enqueue skipped (duplicate/invalid): " + url);
            }
        }

        std::string response;
        if (ok) {
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nqueued\n";
        } else {
            response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nmissing url\n";
        }
        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    }

    close(server_fd);
}

} // namespace

int main(int argc, char* argv[]) {
    // Initialize logger with colors
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_color_output(true);

    try {
        // Load configuration from config.json or command line
        CrawlerConfig config;
        
        // Try to load from config.json if it exists
        std::ifstream config_file("config.json");
        if (config_file.good()) {
            config = ConfigLoader::load("config.json");
        } else {
            config = ConfigLoader::get_default();
        }
        
        // Override with command line arguments
        if (argc > 1) {
            config = ConfigLoader::from_command_line(argc, argv);
        }

        log_info("=== Dataset Crawler for AI ===");
        log_info("C++ Web Crawler with Ethical Crawling Support");

        // Create crawler instance
        WebCrawler crawler(config.user_agent);
        crawler.set_timeout(config.timeout);

        std::atomic<bool> stop_flag(false);
        g_stop_flag = &stop_flag;
        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);
        crawler.set_stop_flag(&stop_flag);
        
        // Enable robots.txt and meta-tag checking
        crawler.set_respect_robots_txt(config.respect_robots_txt);
        crawler.set_respect_meta_tags(config.respect_meta_tags);

        // Configure HTTP/2 support with BoringSSL
        HTTPConfig http_config;
        http_config.enable_http2 = true;              // Enable HTTP/2
        http_config.verify_ssl_cert = false;         // Don't verify certs for now
        http_config.verify_ssl_host = false;         // Don't verify hostname
        http_config.enable_http_keep_alive = true;   // Enable connection reuse
        crawler.set_http_config(http_config);
        
        log_info("HTTP/2 support enabled (with HTTP/1.1 fallback via BoringSSL)");

        crawler.set_headless_rendering(config.enable_headless_rendering,
                                       config.chrome_path,
                                       config.chrome_timeout_seconds);

        ClickHouseConfig clickhouse_config;
        clickhouse_config.enabled = config.clickhouse_enabled;
        clickhouse_config.endpoint = config.clickhouse_endpoint;
        clickhouse_config.database = config.clickhouse_database;
        clickhouse_config.metrics_table = config.clickhouse_metrics_table;
        clickhouse_config.link_graph_table = config.clickhouse_link_graph_table;
        clickhouse_config.user = config.clickhouse_user;
        clickhouse_config.password = config.clickhouse_password;
        clickhouse_config.timeout_seconds = config.clickhouse_timeout_seconds;
        crawler.set_clickhouse_config(clickhouse_config);

        // Enable periodic statistics reporting (every minute)
        crawler.enable_periodic_stats(true);
        
        // Enable deduplication using SimHash
        crawler.enable_deduplication(true);

        // Add custom headers
        for (const auto& [key, value] : config.headers) {
            crawler.add_header(key, value);
        }

        // Display configuration
        std::ostringstream config_msg;
        config_msg << "Configuration: " << config.urls.size() << " URLs, "
                   << "timeout: " << config.timeout << "s, "
                   << "robots.txt: " << (config.respect_robots_txt ? "YES" : "NO") << ", "
                   << "meta-tags: " << (config.respect_meta_tags ? "YES" : "NO");
        log_info(config_msg.str());

        std::vector<std::string> initial_urls = config.urls;
        if (config.api_enabled) {
            if (!initial_urls.empty()) {
                log_warn("API mode enabled; ignoring initial URLs.");
            }
            initial_urls.clear();
        }

        std::thread api_thread;
        if (config.api_enabled) {
            api_thread = std::thread(run_api_server, &crawler, config, &stop_flag);
        }

        // Crawl URLs
        auto records = crawler.crawl_urls(initial_urls, config.api_enabled);

        stop_flag.store(true);
        if (api_thread.joinable()) {
            api_thread.join();
        }

        // Write to files
        ParquetDatasetWriter writer;

        if (!config.output_dir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(config.output_dir, ec);
            if (ec) {
                std::ostringstream warn_msg;
                warn_msg << "Failed to create output directory " << config.output_dir
                         << ": " << ec.message();
                log_warn(warn_msg.str());
            }
        }
        
        if (config.output_format == "json" || config.output_format == "both") {
            std::string json_path = config.output_dir + "/dataset.json";
            writer.write_records(json_path, records);
        }
        
        if (config.output_format == "csv" || config.output_format == "both") {
            std::string csv_path = config.output_dir + "/dataset.csv";
            writer.write_csv(csv_path, records);
        }

        std::ostringstream summary;
        summary << "Crawling complete. Total records: " << records.size();
        log_info(summary.str());

        g_stop_flag = nullptr;
        return 0;

    } catch (const std::exception& e) {
        std::ostringstream error_msg;
        error_msg << "Fatal error: " << e.what();
        log_error(error_msg.str());
        return 1;
    }
}
