#include "crawler.h"
#include "logger.h"
#include "raw_socket_http.h"
#include <curl/curl.h>
#include <iconv.h>
#include <regex>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <cctype>
#include <set>
#include <cstring>
#include <cstdlib>

// Callback for CURL to write data
static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

WebCrawler::WebCrawler(const std::string& user_agent)
    : user_agent_(user_agent), timeout_(30), respect_robots_txt_(true),
      respect_meta_tags_(true), max_file_size_bytes_(100 * 1024 * 1024),  // 100 MB default
      blocked_by_robots_(0), blocked_by_noindex_(0), skipped_by_size_(0),
      sitemaps_found_(0), duplicates_detected_(0), http2_requests_(0), http11_requests_(0),
      http10_requests_(0), total_bytes_downloaded_(0), 
      total_duration_ms_(0), last_request_duration_ms_(0), latency_ema_ms_(0.0),
      consecutive_failures_(0), consecutive_successes_(0), last_delay_ms_(0),
      enable_periodic_stats_(false), stats_thread_running_(false), 
      enable_deduplication_(false) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    crawl_start_time_ = std::chrono::steady_clock::now();
    http_config_.enable_http2 = true;  // Enable HTTP/2 by default
}

WebCrawler::~WebCrawler() {
    stop_stats_reporter();
    curl_global_cleanup();
}

void WebCrawler::set_timeout(long timeout_seconds) {
    timeout_ = timeout_seconds;
}

void WebCrawler::add_header(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void WebCrawler::set_respect_robots_txt(bool respect) {
    respect_robots_txt_ = respect;
}

void WebCrawler::set_respect_meta_tags(bool respect) {
    respect_meta_tags_ = respect;
}

void WebCrawler::set_max_file_size(size_t size_mb) {
    max_file_size_bytes_ = size_mb * 1024 * 1024;
}

int WebCrawler::get_skipped_by_size_count() const {
    return skipped_by_size_;
}

int WebCrawler::get_sitemaps_found_count() const {
    return sitemaps_found_;
}

CrawlerStats WebCrawler::get_statistics() const {
    CrawlerStats stats;
    stats.total_requests = 0;
    stats.successful_requests = 0;
    stats.failed_requests = 0;
    stats.blocked_by_robots = blocked_by_robots_;
    stats.blocked_by_noindex = blocked_by_noindex_;
    stats.skipped_by_size = skipped_by_size_;
    stats.sitemaps_found = sitemaps_found_;
    stats.duplicates_detected = duplicates_detected_;
    stats.http2_requests = http2_requests_;
    stats.http11_requests = http11_requests_;
    stats.http10_requests = http10_requests_;
    stats.total_bytes_downloaded = total_bytes_downloaded_;
    stats.total_duration_ms = total_duration_ms_;
    stats.avg_request_duration_ms = request_durations_.empty() ? 0 : 
        std::accumulate(request_durations_.begin(), request_durations_.end(), 0LL) / request_durations_.size();
    stats.requests_per_minute = total_duration_ms_ > 0 ? 
        (static_cast<double>(request_durations_.size()) * 60000.0) / total_duration_ms_ : 0;
    return stats;
}

int WebCrawler::get_blocked_by_robots_count() const {
    return blocked_by_robots_;
}

int WebCrawler::get_blocked_by_noindex_count() const {
    return blocked_by_noindex_;
}

std::string WebCrawler::get_domain(const std::string& url) {
    size_t start = url.find("://");
    if (start == std::string::npos) return "";
    
    start += 3;
    size_t end = url.find('/', start);
    if (end == std::string::npos) end = url.length();
    
    return url.substr(start, end - start);
}

std::string WebCrawler::extract_title(const std::string& html) {
    std::regex title_regex("<title>([^<]+)</title>", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, title_regex)) {
        return match[1].str();
    }
    return "No title";
}

bool WebCrawler::check_meta_tags(const std::string& html) {
    // Check for noindex meta tag
    std::regex noindex_regex("meta\\s+name=[\"']robots[\"']\\s+content=[\"']([^\"']*)[\"']", 
                            std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, noindex_regex)) {
        std::string content = match[1].str();
        // Convert to lowercase for comparison
        std::transform(content.begin(), content.end(), content.begin(), ::tolower);
        
        if (content.find("noindex") != std::string::npos) {
            std::cout << "  ⊘ Blocked by meta noindex tag" << std::endl;
            blocked_by_noindex_++;
            return false;
        }
    }
    
    return true;
}



std::string WebCrawler::normalize_user_agent(const std::string& agent) {
    // Keep asterisk as-is for wildcard matching
    if (agent == "*") return agent;
    
    // Remove version suffix: "googlebot/1.2" -> "googlebot"
    // Remove asterisk suffix: "googlebot*" -> "googlebot"
    size_t pos = agent.find_first_of("/*");
    if (pos != std::string::npos) {
        return agent.substr(0, pos);
    }
    return agent;
}

bool WebCrawler::matches_user_agent(const std::string& rule_agent, const std::string& crawler_agent) {
    // Wildcard match (check before normalization)
    if (rule_agent == "*") return true;
    
    std::string norm_rule = normalize_user_agent(rule_agent);
    std::string norm_crawler = normalize_user_agent(crawler_agent);
    
    // Exact match
    if (norm_rule == norm_crawler) return true;
    
    // Partial match (case-insensitive)
    std::string rule_lower = norm_rule;
    std::string crawler_lower = norm_crawler;
    std::transform(rule_lower.begin(), rule_lower.end(), rule_lower.begin(), ::tolower);
    std::transform(crawler_lower.begin(), crawler_lower.end(), crawler_lower.begin(), ::tolower);
    
    return rule_lower == crawler_lower;
}

std::vector<RobotRule> WebCrawler::parse_robots_txt(const std::string& /* host */, const std::string& robots_content) {
    std::vector<RobotRule> rules;
    std::istringstream stream(robots_content);
    std::string line;
    RobotRule current_rule;
    bool parsing_rule = false;
    
    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Convert to lowercase for case-insensitive matching
        std::string line_lower = line;
        std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(), ::tolower);
        
        // User-agent directive
        if (line_lower.find("user-agent:") == 0) {
            // Save previous rule if exists
            if (parsing_rule && !current_rule.user_agents.empty()) {
                rules.push_back(current_rule);
            }
            
            // Start new rule
            std::string agent = line.substr(11);
            agent.erase(0, agent.find_first_not_of(" \t"));
            agent.erase(agent.find_last_not_of(" \t\r\n") + 1);
            
            current_rule.user_agents.clear();
            current_rule.disallows.clear();
            current_rule.allows.clear();
            current_rule.user_agents.push_back(agent);
            
            // Calculate specificity
            if (agent == "*") {
                current_rule.specificity = 1;
            } else if (agent.find('*') != std::string::npos || agent.find('/') != std::string::npos) {
                current_rule.specificity = 2;
            } else {
                current_rule.specificity = 3;  // Exact match has highest priority
            }
            
            parsing_rule = true;
        }
        // Disallow directive
        else if (parsing_rule && line_lower.find("disallow:") == 0) {
            std::string path = line.substr(9);
            path.erase(0, path.find_first_not_of(" \t"));
            path.erase(path.find_last_not_of(" \t\r\n") + 1);
            
            if (!path.empty()) {
                current_rule.disallows.push_back(path);
            }
        }
        // Allow directive
        else if (parsing_rule && line_lower.find("allow:") == 0) {
            std::string path = line.substr(6);
            path.erase(0, path.find_first_not_of(" \t"));
            path.erase(path.find_last_not_of(" \t\r\n") + 1);
            
            if (!path.empty()) {
                current_rule.allows.push_back(path);
            }
        }
    }
    
    // Don't forget last rule
    if (parsing_rule && !current_rule.user_agents.empty()) {
        rules.push_back(current_rule);
    }
    
    return rules;
}

bool WebCrawler::is_path_allowed(const std::vector<RobotRule>& rules, const std::string& path) {
    std::vector<RobotRule> matching_rules;
    
    // Find all rules that match our user-agent
    for (const auto& rule : rules) {
        for (const auto& agent : rule.user_agents) {
            if (matches_user_agent(agent, user_agent_)) {
                matching_rules.push_back(rule);
                break;
            }
        }
    }
    
    if (matching_rules.empty()) {
        return true;  // No rules found, allow everything
    }
    
    // Sort by specificity (higher specificity first)
    std::sort(matching_rules.begin(), matching_rules.end(),
              [](const RobotRule& a, const RobotRule& b) {
                  return a.specificity > b.specificity;
              });
    
    // Combine rules from all matching non-wildcard rules, then add wildcard rules
    std::vector<std::string> combined_disallows;
    std::vector<std::string> combined_allows;
    
    for (auto& rule : matching_rules) {
        bool is_wildcard = false;
        for (const auto& agent : rule.user_agents) {
            if (agent == "*") {
                is_wildcard = true;
                break;
            }
        }
        
        // Only include non-wildcard rules in combination
        if (!is_wildcard) {
            combined_disallows.insert(combined_disallows.end(),
                                     rule.disallows.begin(), rule.disallows.end());
            combined_allows.insert(combined_allows.end(),
                                  rule.allows.begin(), rule.allows.end());
        }
    }
    
    // If no specific rules, add wildcard rules
    if (combined_disallows.empty() && combined_allows.empty()) {
        for (auto& rule : matching_rules) {
            bool is_wildcard = false;
            for (const auto& agent : rule.user_agents) {
                if (agent == "*") {
                    is_wildcard = true;
                    break;
                }
            }
            if (is_wildcard) {
                combined_disallows.insert(combined_disallows.end(),
                                         rule.disallows.begin(), rule.disallows.end());
                combined_allows.insert(combined_allows.end(),
                                      rule.allows.begin(), rule.allows.end());
            }
        }
    }
    
    // Find the longest matching allow and disallow rules
    std::string best_allow;
    std::string best_disallow;
    size_t best_allow_len = 0;
    size_t best_disallow_len = 0;
    
    for (const auto& allow_path : combined_allows) {
        if (match_path_pattern(allow_path, path)) {
            // Remove $ suffix for length comparison if present
            std::string pattern_for_length = allow_path;
            if (!pattern_for_length.empty() && pattern_for_length.back() == '$') {
                pattern_for_length.pop_back();
            }
            if (pattern_for_length.length() > best_allow_len) {
                best_allow_len = pattern_for_length.length();
                best_allow = allow_path;
            }
        }
    }
    
    for (const auto& disallow_path : combined_disallows) {
        if (disallow_path == "/" || match_path_pattern(disallow_path, path)) {
            // Remove $ suffix for length comparison if present
            std::string pattern_for_length = disallow_path;
            if (!pattern_for_length.empty() && pattern_for_length.back() == '$') {
                pattern_for_length.pop_back();
            }
            if (pattern_for_length.length() > best_disallow_len) {
                best_disallow_len = pattern_for_length.length();
                best_disallow = disallow_path;
            }
        }
    }
    
    // If no rules matched, allow the path
    if (best_allow.empty() && best_disallow.empty()) {
        return true;
    }
    
    // If only allow matched, return true
    if (!best_allow.empty() && best_disallow.empty()) {
        return true;
    }
    
    // If only disallow matched, return false
    if (best_allow.empty() && !best_disallow.empty()) {
        return false;
    }
    
    // Both matched - use longest match
    if (best_allow_len > best_disallow_len) {
        return true;
    } else if (best_disallow_len > best_allow_len) {
        return false;
    } else {
        // Equal length - allow wins (least restrictive)
        return true;
    }
}

// Overloaded version that accepts user-agent parameter
bool WebCrawler::is_path_allowed(const std::vector<RobotRule>& rules, const std::string& path, const std::string& user_agent) {
    // Temporarily set the user agent to the provided one
    std::string old_user_agent = user_agent_;
    user_agent_ = user_agent;
    
    // Call the original function
    bool result = is_path_allowed(rules, path);
    
    // Restore the original user agent
    user_agent_ = old_user_agent;
    
    return result;
}

bool WebCrawler::match_path_pattern(const std::string& pattern, const std::string& path) {
    // Handle empty pattern (should not match anything except empty path)
    if (pattern.empty()) {
        return path.empty();
    }
    
    // Check for $ end marker
    bool has_end_marker = !pattern.empty() && pattern.back() == '$';
    std::string actual_pattern = has_end_marker ? pattern.substr(0, pattern.length() - 1) : pattern;
    
    // Simple case: no wildcards
    size_t wildcard_pos = actual_pattern.find('*');
    if (wildcard_pos == std::string::npos) {
        // No wildcard - just prefix matching
        if (path.find(actual_pattern) == 0) {
            // If has end marker, must match entire path
            if (has_end_marker) {
                return path == actual_pattern;
            }
            return true;
        }
        return false;
    }
    
    // Handle wildcards - use regex matching for flexibility
    std::string regex_pattern = actual_pattern;
    
    // Escape special regex characters except *
    std::string escaped;
    for (char c : regex_pattern) {
        if (c == '*') {
            escaped += ".*";  // * matches 0 or more of any character
        } else if (c == '.' || c == '+' || c == '?' || c == '[' || c == ']' || 
                   c == '(' || c == ')' || c == '{' || c == '}' || c == '^' || c == '|') {
            escaped += '\\';
            escaped += c;
        } else {
            escaped += c;
        }
    }
    
    if (has_end_marker) {
        escaped += '$';  // End of string anchor
    }
    
    try {
        std::regex re(escaped);
        return std::regex_match(path, re);
    } catch (...) {
        // Fallback if regex is invalid
        return false;
    }
}

bool WebCrawler::check_robots_txt(const std::string& url) {
    std::string domain = get_domain(url);
    if (domain.empty()) return true;
    
    // Extract path from URL
    size_t path_start = url.find('/', url.find("://") + 3);
    std::string path = (path_start != std::string::npos) ? url.substr(path_start) : "/";
    
    // Check cache for parsed rules
    if (robots_rules_cache_.find(domain) != robots_rules_cache_.end()) {
        return is_path_allowed(robots_rules_cache_[domain], path);
    }
    
    // Fetch robots.txt
    std::string robots_url = "https://" + domain + "/robots.txt";
    int status_code = 0;
    std::string robots_content = fetch_html(robots_url, status_code);
    
    std::vector<RobotRule> rules;
    
    if (status_code == 200) {
        rules = parse_robots_txt(domain, robots_content);
    } else if (status_code != 404) {
        // Log warning for failed robots.txt fetch (but not 404)
        std::ostringstream warn_msg;
        warn_msg << "Failed to fetch robots.txt for request " << url;
        log_warn(warn_msg.str());
    }
    
    // Cache the rules
    robots_rules_cache_[domain] = rules;
    
    return is_path_allowed(rules, path);
}

std::string WebCrawler::fetch_html(const std::string& url, int& status_code) {
    auto request_start = std::chrono::steady_clock::now();

    std::string response;
    std::string content_type;
    std::string scheme;
    auto scheme_pos = url.find("://");
    if (scheme_pos != std::string::npos) {
        scheme = url.substr(0, scheme_pos);
    }

    if (http_config_.use_raw_sockets && scheme == "http") {
        RawSocketHttpConfig raw_config;
        raw_config.timeout = std::chrono::seconds(timeout_);
        raw_config.retry.max_retries = http_config_.max_retries;
        raw_config.retry.retry_backoff_ms = http_config_.retry_backoff_ms;

        std::map<std::string, std::string> request_headers;
        request_headers["Accept"] = "text/html,application/xhtml+xml";
        request_headers["Accept-Language"] = "en-US,en;q=0.9";
        request_headers["Accept-Encoding"] = "identity";
        for (const auto& [key, value] : headers_) {
            request_headers[key] = value;
        }

        RawSocketHttpClient client(raw_config);
        RawHttpResponse raw_response = client.fetch(url, request_headers);
        response = raw_response.body;
        content_type = raw_response.content_type;
        status_code = raw_response.status_code;

        switch (raw_response.http_version) {
            case HTTPVersion::HTTP_1_0:
                http10_requests_++;
                break;
            case HTTPVersion::HTTP_1_1:
                http11_requests_++;
                break;
            case HTTPVersion::HTTP_2_0:
                http2_requests_++;
                break;
            default:
                break;
        }

        if (!raw_response.success) {
            log_error("Raw socket error for " + url + ": " + raw_response.error_message);
        }
    } else {
        int attempts = std::max(1, http_config_.max_retries + 1);
        for (int attempt = 0; attempt < attempts; ++attempt) {
            CURL* curl = curl_easy_init();
            if (!curl) {
                std::cerr << "Failed to initialize CURL" << std::endl;
                status_code = 0;
                return "";
            }

            response.clear();
            content_type.clear();
            struct curl_slist* headers = nullptr;

            // Add default headers
            headers = curl_slist_append(headers, "User-Agent: DatasetCrawler/1.0");
            headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml");
            headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
            headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");

            // Add custom headers
            for (const auto& [key, value] : headers_) {
                std::string header = key + ": " + value;
                headers = curl_slist_append(headers, header.c_str());
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");

            // Enable HTTP/2 support (automatically falls back to HTTP/1.1 if HTTP/2 not available)
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

            // SSL/TLS configuration for BoringSSL
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Accept any cert for now
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);  // Don't verify hostname

            // Enable connection reuse
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::string error_msg = std::string(curl_easy_strerror(res));
                if (error_msg.find("Unsupported") != std::string::npos ||
                    error_msg.find("Invalid") != std::string::npos ||
                    error_msg.find("malformed") != std::string::npos) {
                    log_warn("Failed to parse URL: " + error_msg);
                } else {
                    log_error("CURL error for " + url + ": " + error_msg);
                }
                status_code = 0;
            } else {
                long http_code = 0;
                char* final_url = nullptr;
                char* content_type_ptr = nullptr;
                long http_version = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &final_url);
                curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type_ptr);
                curl_easy_getinfo(curl, CURLINFO_HTTP_VERSION, &http_version);

                if (content_type_ptr) {
                    content_type = std::string(content_type_ptr);
                }

                std::string http_version_str;
                switch (http_version) {
                    case CURL_HTTP_VERSION_1_0:
                        http_version_str = "HTTP/1.0";
                        http10_requests_++;
                        break;
                    case CURL_HTTP_VERSION_1_1:
                        http_version_str = "HTTP/1.1";
                        http11_requests_++;
                        break;
                    case CURL_HTTP_VERSION_2_0:
                        http_version_str = "HTTP/2";
                        http2_requests_++;
                        break;
                    default:
                        http_version_str = "HTTP/?.?";
                }

                if (final_url && final_url != url) {
                    std::string final_url_str(final_url);
                    if (final_url_str != url) {
                        std::ostringstream redirect_msg;
                        redirect_msg << "The start URL \"" << url << "\" has been redirected to \""
                                    << final_url_str << "\" [" << http_version_str << "]";
                        log_warn(redirect_msg.str());
                    }
                }

                status_code = static_cast<int>(http_code);
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (status_code > 0) {
                break;
            }

            if (attempt + 1 < attempts) {
                int backoff = http_config_.retry_backoff_ms * (attempt + 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
            }
        }
    }
    
    // Detect and convert encoding
    std::string encoding = detect_encoding(response, content_type);
    if (encoding != "UTF-8" && encoding != "UTF8") {
        std::ostringstream conv_msg;
        conv_msg << "Converting content from " << encoding << " to UTF-8";
        log_info(conv_msg.str());
        response = convert_to_utf8(response, encoding);
    }
    
    // Track request duration and bytes
    auto request_end = std::chrono::steady_clock::now();
    long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        request_end - request_start).count();
    last_request_duration_ms_ = duration_ms;
    request_durations_.push_back(duration_ms);
    total_duration_ms_ += duration_ms;
    total_bytes_downloaded_ += response.length();

    return response;
}

DataRecord WebCrawler::fetch(const std::string& url) {
    int status_code = 0;
    
    // Check robots.txt if enabled
    if (respect_robots_txt_) {
        if (!check_robots_txt(url)) {
            blocked_by_robots_++;
            
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            
            DataRecord blocked_record;
            blocked_record.url = url;
            blocked_record.title = "BLOCKED";
            blocked_record.content = "";
            blocked_record.timestamp = oss.str();
            blocked_record.status_code = 403;
            blocked_record.was_allowed = false;
            blocked_record.content_length = 0;
            blocked_record.was_skipped = false;
            return blocked_record;
        }
    }
    
    std::string html = fetch_html(url, status_code);
    
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    DataRecord record;
    record.url = url;
    record.title = extract_title(html);
    record.content = html;
    record.timestamp = oss.str();
    record.status_code = status_code;
    record.was_allowed = true;
    record.content_length = html.length();
    record.was_skipped = false;
    
    // Check file size
    if (html.length() > max_file_size_bytes_) {
        std::ostringstream warn_msg;
        warn_msg << "Skipped " << url << " - file size " 
                 << (html.length() / 1024 / 1024) << "MB exceeds limit";
        log_warn(warn_msg.str());
        skipped_by_size_++;
        record.was_skipped = true;
        record.was_allowed = false;
        return record;
    }
    
    // Check for "No text parsed" condition
    if (html.length() < 100 && status_code == 200) {
        std::ostringstream warn_msg;
        warn_msg << "No text parsed from " << url << ".";
        log_warn(warn_msg.str());
    }
    
    // Check meta tags if enabled
    if (respect_meta_tags_ && status_code == 200) {
        if (!check_meta_tags(html)) {
            record.was_allowed = false;
            return record;
        }
    }

    // Check for duplicates if deduplication is enabled
    if (enable_deduplication_ && status_code == 200 && html.length() > 100) {
        uint64_t content_hash = calculate_simhash(html);
        if (is_duplicate(content_hash, 3)) {  // threshold = 3 bits difference
            std::ostringstream dup_msg;
            dup_msg << "Duplicate content detected for " << url;
            log_warn(dup_msg.str());
            record.was_allowed = false;
            record.was_skipped = true;
        }
    }

    return record;
}

std::vector<DataRecord> WebCrawler::crawl_urls(const std::vector<std::string>& urls) {
    std::vector<DataRecord> records;
    constexpr int kInitialPriority = 0;
    constexpr int kDiscoveredPriority = 1;
    
    // Clear memory caches
    visited_urls_memory_.clear();
    
    // Initialize database for persistent queue storage
    if (!db_manager_ || !db_manager_->init()) {
        log_error("Failed to initialize RocksDB for queue management");
        return records;
    }
    
    // Enqueue initial URLs to RocksDB
    for (const auto& url : urls) {
        db_manager_->enqueue_url(normalize_url(url), kInitialPriority);
    }
    
    // Log start of crawling
    std::ostringstream start_msg;
    start_msg << "Crawling will be started using " << urls.size() 
              << " start URLs (stored in RocksDB)";
    log_info(start_msg.str());
    
    log_info("Starting the crawler with RocksDB-based queue management.");
    
    auto crawl_start = std::chrono::steady_clock::now();
    
    // Process URLs from RocksDB queue
    while (db_manager_->has_queued_urls()) {
        std::string url = db_manager_->dequeue_url();
        
        if (url.empty()) {
            break;
        }
        
        // Skip if already visited (check both memory cache and RocksDB)
        std::string normalized = normalize_url(url);
        if (visited_urls_memory_.find(normalized) != visited_urls_memory_.end() || db_manager_->is_visited(normalized)) {
            continue;
        }
        
        // Mark as visited in both memory and RocksDB
        visited_urls_memory_.insert(normalized);
        db_manager_->mark_visited(normalized);
        
        try {
            DataRecord record = fetch(url);
            
            if (record.was_allowed && !record.was_skipped) {
                records.push_back(record);
                
                if (record.status_code == 200) {
                    std::ostringstream success_msg;
                    success_msg << url << " [" << record.status_code << "]";
                    log_info(success_msg.str());
                    
                    // Extract links from the page
                    std::vector<std::string> new_links = extract_links_from_html(record.content, url);

                    // Store link graph edges
                    for (const auto& link : new_links) {
                        db_manager_->add_link_edge(normalized, link);
                    }
                    
                    // Filter out already visited links and enqueue to RocksDB
                    std::vector<std::string> unvisited_links;
                    for (const auto& link : new_links) {
                        if (visited_urls_memory_.find(link) == visited_urls_memory_.end() && !db_manager_->is_visited(link)) {
                            unvisited_links.push_back(link);
                            db_manager_->enqueue_url(link, kDiscoveredPriority);  // Persist to RocksDB
                        }
                    }
                    
                    // Log enqueued links
                    if (!unvisited_links.empty()) {
                        std::ostringstream enqueue_msg;
                        enqueue_msg << "Enqueued " << unvisited_links.size() << " new links on " << url;
                        log_info(enqueue_msg.str());
                    }
                } else {
                    std::ostringstream error_msg;
                    error_msg << url << " [" << record.status_code << "]";
                    log_warn(error_msg.str());
                }
            } else if (record.was_skipped) {
                std::ostringstream skip_msg;
                skip_msg << url << " [skipped - size limit exceeded]";
                log_warn(skip_msg.str());
            } else {
                std::ostringstream blocked_msg;
                blocked_msg << url << " [blocked]";
                log_warn(blocked_msg.str());
            }

            apply_adaptive_delay(record.status_code);
        } catch (const std::exception& e) {
            std::string error_str = e.what();
            // Check if it's a URL parsing error
            if (error_str.find("Invalid") != std::string::npos || 
                error_str.find("URL") != std::string::npos ||
                error_str.find("parse") != std::string::npos) {
                std::ostringstream error_msg;
                error_msg << "Failed to parse URL: " << error_str;
                log_warn(error_msg.str());
            } else {
                std::ostringstream error_msg;
                error_msg << url << " - " << error_str;
                log_error(error_msg.str());
            }

            apply_adaptive_delay(0);
        }
    }
    
    auto crawl_end = std::chrono::steady_clock::now();
    long crawl_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        crawl_end - crawl_start).count();
    
    // Log completion statistics
    std::ostringstream final_msg;
    final_msg << "Crawling completed. Fetched: " << records.size() << " records, "
              << "Blocked by robots.txt: " << blocked_by_robots_ << ", "
              << "Blocked by noindex: " << blocked_by_noindex_ << ", "
              << "Skipped by size: " << skipped_by_size_;
    log_info(final_msg.str());
    
    // Calculate statistics
    double avg_duration = 0.0;
    double requests_per_minute = 0.0;
    if (!request_durations_.empty()) {
        long sum = 0;
        for (auto d : request_durations_) {
            sum += d;
        }
        avg_duration = (double)sum / request_durations_.size();
        requests_per_minute = request_durations_.size() * 60000.0 / crawl_duration_ms;
    }
    
    // Log detailed statistics in JSON format
    std::ostringstream stats_msg;
    stats_msg << "Statistics: request statistics: {\"requestAvgFailedDurationMillis\":null,"
              << "\"requestAvgFinishedDurationMillis\":" << (long)avg_duration << ","
              << "\"requestsFinishedPerMinute\":" << (int)requests_per_minute << ","
              << "\"requestsFailedPerMinute\":" 
              << (int)((blocked_by_robots_ + blocked_by_noindex_) * 60000.0 / crawl_duration_ms) << ","
              << "\"requestTotalDurationMillis\":" << crawl_duration_ms << ","
              << "\"requestsTotal\":" << (records.size() + blocked_by_robots_ + blocked_by_noindex_ + skipped_by_size_) << ","
              << "\"crawlerRuntimeMillis\":" << crawl_duration_ms << ","
              << "\"retryHistogram\":[" << (records.size() + blocked_by_robots_ + blocked_by_noindex_ + skipped_by_size_) << "]}";
    log_info(stats_msg.str());
    
    return records;
}

std::vector<std::string> WebCrawler::extract_sitemap_urls_from_robots(const std::string& robots_content) {
    std::vector<std::string> sitemap_urls;
    std::istringstream stream(robots_content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Check if line starts with Sitemap:
        if (line.length() > 8 && line.substr(0, 8) == "Sitemap:") {
            std::string url = line.substr(8);
            // Trim whitespace from URL
            url.erase(0, url.find_first_not_of(" \t"));
            url.erase(url.find_last_not_of(" \t") + 1);
            if (!url.empty()) {
                sitemap_urls.push_back(url);
            }
        }
    }
    
    return sitemap_urls;
}

std::vector<std::string> WebCrawler::parse_sitemap_xml(const std::string& xml_content) {
    std::vector<std::string> urls;
    std::regex url_regex("<loc>([^<]+)</loc>");
    std::smatch match;
    
    std::string::const_iterator search_start(xml_content.cbegin());
    while (std::regex_search(search_start, xml_content.cend(), match, url_regex)) {
        urls.push_back(match[1].str());
        search_start = match.suffix().first;
    }
    
    return urls;
}

std::vector<std::string> WebCrawler::get_sitemaps_from_robots(const std::string& domain) {
    // Check cache first
    auto it = robots_sitemaps_cache_.find(domain);
    if (it != robots_sitemaps_cache_.end()) {
        return it->second;
    }
    
    // Fetch robots.txt
    std::string robots_url = "https://" + domain + "/robots.txt";
    int status_code = 0;
    std::string robots_content = fetch_html(robots_url, status_code);
    
    if (status_code != 200 || robots_content.empty()) {
        robots_sitemaps_cache_[domain] = {};
        return {};
    }
    
    // Extract Sitemap URLs
    std::vector<std::string> sitemap_urls = extract_sitemap_urls_from_robots(robots_content);
    sitemaps_found_ += sitemap_urls.size();
    
    // Log found sitemaps
    if (!sitemap_urls.empty()) {
        std::ostringstream msg;
        msg << "Found " << sitemap_urls.size() << " sitemap(s) in robots.txt for " << domain;
        log_info(msg.str());
    }
    
    // Cache the result
    robots_sitemaps_cache_[domain] = sitemap_urls;
    return sitemap_urls;
}

std::vector<std::string> WebCrawler::fetch_sitemap_urls(const std::string& sitemap_url) {
    int status_code = 0;
    std::string sitemap_content = fetch_html(sitemap_url, status_code);
    
    if (status_code != 200 || sitemap_content.empty()) {
        log_warn("Failed to fetch sitemap from " + sitemap_url + " (status: " + std::to_string(status_code) + ")");
        return {};
    }
    
    std::vector<std::string> urls = parse_sitemap_xml(sitemap_content);
    std::ostringstream msg;
    msg << "Parsed " << urls.size() << " URLs from sitemap: " << sitemap_url;
    log_info(msg.str());
    
    return urls;
}

bool WebCrawler::is_valid_url(const std::string& url) {
    if (url.empty()) return false;
    return url.find("http://") == 0 || url.find("https://") == 0;
}

std::string WebCrawler::normalize_url(const std::string& url) {
    if (!is_valid_url(url)) return "";
    
    // Remove fragment (#)
    size_t hash_pos = url.find('#');
    std::string normalized = (hash_pos != std::string::npos) ? url.substr(0, hash_pos) : url;
    
    // Convert to lowercase (scheme and host only)
    size_t scheme_end = normalized.find("://");
    if (scheme_end != std::string::npos) {
        size_t path_start = normalized.find('/', scheme_end + 3);
        if (path_start == std::string::npos) {
            path_start = normalized.length();
        }
        std::string host_part = normalized.substr(0, path_start);
        std::transform(host_part.begin(), host_part.end(), host_part.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        normalized = host_part + normalized.substr(path_start);
    }
    
    // Remove trailing slash if not root
    if (normalized.length() > 1 && normalized.back() == '/' && 
        normalized.find('/', 8) != normalized.length() - 1) {
        // Keep root slash but remove trailing slashes on paths
        size_t last_slash = normalized.rfind('/');
        if (last_slash > 8) {  // 8 = length of "https://"
            normalized = normalized.substr(0, normalized.length() - 1);
        }
    }
    
    return normalized;
}

std::string WebCrawler::resolve_relative_url(const std::string& base_url, const std::string& relative_url) {
    if (relative_url.empty()) return "";
    
    // Already absolute URL
    if (is_valid_url(relative_url)) {
        return normalize_url(relative_url);
    }
    
    // Protocol-relative URL (//domain.com/path)
    if (relative_url.substr(0, 2) == "//") {
        size_t scheme_end = base_url.find("://");
        if (scheme_end != std::string::npos) {
            std::string scheme = base_url.substr(0, scheme_end);
            std::string result = scheme + ":" + relative_url;
            return normalize_url(result);
        }
        return "";
    }
    
    // Parse base URL
    size_t scheme_end = base_url.find("://");
    if (scheme_end == std::string::npos) return "";
    
    size_t host_start = scheme_end + 3;
    size_t path_start = base_url.find('/', host_start);
    if (path_start == std::string::npos) path_start = base_url.length();
    
    std::string scheme = base_url.substr(0, scheme_end + 3);
    std::string host = base_url.substr(host_start, path_start - host_start);
    
    std::string result;
    std::string path;
    
    if (relative_url[0] == '/') {
        // Absolute path
        path = relative_url;
    } else {
        // Relative path - start from directory of base URL
        std::string base_path = base_url.substr(path_start);
        size_t last_slash = base_path.rfind('/');
        if (last_slash != std::string::npos) {
            base_path = base_path.substr(0, last_slash + 1);
        } else {
            base_path = "/";
        }
        path = base_path + relative_url;
    }
    
    // Resolve .. and . in path
    std::vector<std::string> segments;
    std::istringstream iss(path);
    std::string segment;
    
    while (std::getline(iss, segment, '/')) {
        if (segment == "." || segment.empty()) {
            continue;
        } else if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else {
            segments.push_back(segment);
        }
    }
    
    // Rebuild path
    std::string resolved_path = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        resolved_path += segments[i];
        if (i < segments.size() - 1 || path.back() == '/') {
            resolved_path += "/";
        }
    }
    
    result = scheme + host + resolved_path;
    return normalize_url(result);
}

std::string WebCrawler::extract_canonical_url(const std::string& html, const std::string& base_url) {
    // Look for <link rel="canonical" href="...">
    std::regex canonical_regex(R"(<link\s+[^>]*rel=["\']?canonical["\']?[^>]*href=["\']([^"\']+)["\'][^>]*>)", 
                               std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, canonical_regex)) {
        std::string canonical = match[1].str();
        return resolve_relative_url(base_url, canonical);
    }
    
    return "";
}

std::vector<std::string> WebCrawler::extract_links_from_html(const std::string& html, const std::string& base_url) {
    std::vector<std::string> links;
    std::set<std::string> unique_links;  // To avoid duplicates
    
    // Extract all href attributes from <a> tags
    std::regex link_regex(R"(href=["\']([^"\']+)["\'])", std::regex::icase);
    
    auto links_begin = std::sregex_iterator(html.begin(), html.end(), link_regex);
    auto links_end = std::sregex_iterator();
    
    for (auto it = links_begin; it != links_end; ++it) {
        std::string url = it->str(1);
        
        // Skip certain URLs
        if (url.empty() || url[0] == '#' || url.find("javascript:") == 0 || 
            url.find("mailto:") == 0 || url.find("tel:") == 0) {
            continue;
        }
        
        // Resolve relative URLs
        std::string resolved = resolve_relative_url(base_url, url);
        if (!resolved.empty()) {
            std::string normalized = normalize_url(resolved);
            if (!normalized.empty() && is_valid_url(normalized)) {
                unique_links.insert(normalized);
            }
        }
    }
    
    // Also check for canonical URL (it's the preferred URL for a page)
    std::string canonical = extract_canonical_url(html, base_url);
    if (!canonical.empty() && unique_links.find(canonical) == unique_links.end()) {
        unique_links.insert(canonical);
    }
    
    // Convert set to vector
    links.assign(unique_links.begin(), unique_links.end());
    
    return links;
}

/**
 * Detect encoding from Content-Type header and HTML meta tags
 */
std::string WebCrawler::detect_encoding(const std::string& content, const std::string& content_type) {
    std::string encoding = "UTF-8";  // Default encoding
    
    // Try to extract encoding from Content-Type header
    // Format: text/html; charset=utf-8
    size_t charset_pos = content_type.find("charset=");
    if (charset_pos != std::string::npos) {
        size_t start = charset_pos + 8;
        size_t end = content_type.find(';', start);
        if (end == std::string::npos) {
            end = content_type.length();
        }
        encoding = content_type.substr(start, end - start);
        
        // Trim whitespace and quotes
        size_t first = encoding.find_first_not_of(" \t\"'");
        size_t last = encoding.find_last_not_of(" \t\"'");
        if (first != std::string::npos) {
            encoding = encoding.substr(first, (last - first + 1));
        }
        
        // Convert to uppercase for normalization
        std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::toupper);
        return encoding;
    }
    
    // Try to extract encoding from meta charset tag
    std::regex meta_charset_regex(R"(<meta\s+charset\s*=\s*[\"']?([^\s\"'>]+)[\"']?)", 
                                   std::regex::icase);
    std::smatch match;
    if (std::regex_search(content, match, meta_charset_regex)) {
        encoding = match[1].str();
        std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::toupper);
        return encoding;
    }
    
    // Try alternative meta tag format
    std::regex meta_http_equiv_regex(R"(<meta\s+http-equiv\s*=\s*[\"']?content-type[\"']?\s+content\s*=\s*[\"']([^\"']*)[\"'])",
                                     std::regex::icase);
    if (std::regex_search(content, match, meta_http_equiv_regex)) {
        std::string content_type_meta = match[1].str();
        size_t charset_pos = content_type_meta.find("charset=");
        if (charset_pos != std::string::npos) {
            encoding = content_type_meta.substr(charset_pos + 8);
            size_t end = encoding.find(';');
            if (end != std::string::npos) {
                encoding = encoding.substr(0, end);
            }
            std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::toupper);
            return encoding;
        }
    }
    
    return encoding;
}

/**
 * Convert content from source encoding to UTF-8
 */
std::string WebCrawler::convert_to_utf8(const std::string& content, const std::string& from_encoding) {
    if (content.empty()) {
        return content;
    }
    
    std::string normalized_encoding = from_encoding;
    std::transform(normalized_encoding.begin(), normalized_encoding.end(), 
                   normalized_encoding.begin(), ::toupper);
    
    // If already UTF-8, return as is
    if (normalized_encoding == "UTF-8" || normalized_encoding == "UTF8") {
        return content;
    }
    
    // Open iconv conversion descriptor
    iconv_t cd = iconv_open("UTF-8", normalized_encoding.c_str());
    if (cd == (iconv_t)(-1)) {
        std::ostringstream err_msg;
        err_msg << "Unsupported encoding: " << normalized_encoding << ", keeping original content";
        log_warn(err_msg.str());
        return content;
    }
    
    // Allocate output buffer (use 4x input size to be safe)
    size_t in_bytes = content.length();
    size_t out_size = in_bytes * 4 + 1;
    char* out_buf = new char[out_size];
    
    const char* in_ptr = content.c_str();
    char* out_ptr = out_buf;
    size_t in_left = in_bytes;
    size_t out_left = out_size - 1;
    
    // Convert the string
    size_t result = iconv(cd, (char**)&in_ptr, &in_left, &out_ptr, &out_left);
    
    std::string converted;
    if (result == (size_t)(-1)) {
        // Conversion error - log it but try to use what we got
        std::ostringstream err_msg;
        err_msg << "Iconv conversion error from " << normalized_encoding << " to UTF-8";
        log_warn(err_msg.str());
        converted.assign(out_buf, out_ptr - out_buf);
    } else {
        converted.assign(out_buf, out_ptr - out_buf);
    }
    
    // Cleanup
    iconv_close(cd);
    delete[] out_buf;
    
    return converted;
}

void WebCrawler::apply_adaptive_delay(int status_code) {
    if (!http_config_.enable_adaptive_delay) {
        return;
    }

    bool success = status_code >= 200 && status_code < 400;
    if (success) {
        consecutive_successes_++;
        consecutive_failures_ = 0;
    } else {
        consecutive_failures_++;
        consecutive_successes_ = 0;
    }

    double latency_sample = static_cast<double>(
        last_request_duration_ms_ > 0 ? last_request_duration_ms_ : http_config_.base_delay_ms);
    if (latency_ema_ms_ == 0.0) {
        latency_ema_ms_ = latency_sample;
    } else {
        latency_ema_ms_ = http_config_.latency_ema_alpha * latency_sample +
            (1.0 - http_config_.latency_ema_alpha) * latency_ema_ms_;
    }

    int queue_size = 0;
    if (db_manager_) {
        queue_size = db_manager_->get_queue_size();
    }
    double queue_pressure = std::min(1.0, static_cast<double>(queue_size) / 1000.0);
    double queue_adjust = 1.0 - (0.3 * queue_pressure);

    int latency_based = static_cast<int>(latency_ema_ms_ * 0.6);
    int base_delay = std::max(http_config_.base_delay_ms, latency_based);
    int delay_ms = static_cast<int>(base_delay * queue_adjust);

    if (!success) {
        delay_ms += http_config_.failure_backoff_ms * consecutive_failures_;
    } else if (consecutive_successes_ > 3) {
        delay_ms = static_cast<int>(delay_ms * 0.8);
    }

    if (last_delay_ms_ > 0) {
        delay_ms = static_cast<int>(0.7 * last_delay_ms_ + 0.3 * delay_ms);
    }

    delay_ms = std::max(http_config_.min_delay_ms, std::min(delay_ms, http_config_.max_delay_ms));

    int jitter_range = std::max(0, delay_ms * http_config_.jitter_pct / 100);
    if (jitter_range > 0) {
        int jitter = (std::rand() % (2 * jitter_range + 1)) - jitter_range;
        delay_ms = std::max(http_config_.min_delay_ms,
                            std::min(delay_ms + jitter, http_config_.max_delay_ms));
    }

    last_delay_ms_ = delay_ms;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
}

/**
 * Enable periodic statistics reporting (every minute)
 */
void WebCrawler::enable_periodic_stats(bool enable) {
    enable_periodic_stats_ = enable;
    if (enable) {
        start_stats_reporter();
    } else {
        stop_stats_reporter();
    }
}

bool WebCrawler::is_periodic_stats_enabled() const {
    return enable_periodic_stats_;
}

/**
 * Start background thread for periodic statistics reporting
 */
void WebCrawler::start_stats_reporter() {
    if (stats_thread_running_) {
        return;  // Already running
    }
    
    stats_thread_running_ = true;
    stats_reporter_thread_ = std::thread(&WebCrawler::stats_reporter_loop, this);
}

/**
 * Stop background thread for periodic statistics reporting
 */
void WebCrawler::stop_stats_reporter() {
    if (!stats_thread_running_) {
        return;
    }
    
    stats_thread_running_ = false;
    if (stats_reporter_thread_.joinable()) {
        stats_reporter_thread_.join();
    }
}

/**
 * Background thread loop for periodic statistics reporting
 */
void WebCrawler::stats_reporter_loop() {
    const int REPORT_INTERVAL_MS = 60000;  // 60 seconds = 1 minute
    
    while (stats_thread_running_) {
        // Sleep for the reporting interval
        std::this_thread::sleep_for(std::chrono::milliseconds(REPORT_INTERVAL_MS));
        
        if (!stats_thread_running_) {
            break;  // Check flag again after sleep
        }
        
        // Get current statistics
        CrawlerStats stats = get_statistics();
        
        // Format and log statistics
        std::string stats_message = format_stats_message(stats);
        log_info(stats_message);
    }
}

/**
 * Format statistics into a readable message
 */
std::string WebCrawler::format_stats_message(const CrawlerStats& stats) {
    std::ostringstream message;
    message << "[STATS REPORT] ";
    message << "Requests: " << stats.total_requests << " | ";
    message << "Success: " << stats.successful_requests << " | ";
    message << "Failed: " << stats.failed_requests << " | ";
    message << "Blocked (robots): " << stats.blocked_by_robots << " | ";
    message << "Blocked (noindex): " << stats.blocked_by_noindex << " | ";
    message << "Skipped (size): " << stats.skipped_by_size << " | ";
    message << "Duplicates: " << stats.duplicates_detected << " | ";
    message << "HTTP/2: " << stats.http2_requests << " | ";
    message << "HTTP/1.1: " << stats.http11_requests << " | ";
    message << "Data: " << (stats.total_bytes_downloaded / (1024 * 1024)) << " MB | ";
    message << "Avg Speed: " << stats.avg_request_duration_ms << " ms/req | ";
    message << "Rate: " << stats.requests_per_minute << " req/min";
    return message.str();
}

/**
 * Enable/disable deduplication using SimHash
 */
void WebCrawler::enable_deduplication(bool enable) {
    enable_deduplication_ = enable;
    if (enable) {
        std::lock_guard<std::mutex> lock(dedup_mutex_);
        content_hashes_.clear();
        log_info("Deduplication (SimHash) enabled");
    } else {
        std::lock_guard<std::mutex> lock(dedup_mutex_);
        content_hashes_.clear();
        log_info("Deduplication (SimHash) disabled");
    }
}

bool WebCrawler::is_deduplication_enabled() const {
    return enable_deduplication_;
}

int WebCrawler::get_duplicates_detected_count() const {
    return duplicates_detected_;
}

/**
 * Calculate SimHash for content deduplication
 * SimHash is a technique for generating a hash that is similar for similar documents
 */
uint64_t WebCrawler::calculate_simhash(const std::string& content) {
    if (content.empty()) {
        return 0;
    }
    
    // Create a vector to hold token hashes
    std::vector<uint64_t> hashes;
    
    // Tokenize the content into words (shingles)
    std::string token;
    for (char c : content) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                // Simple hash of the token
                uint64_t hash = 0;
                for (char ch : token) {
                    hash = hash * 31 + static_cast<unsigned char>(ch);
                }
                hashes.push_back(hash);
                token.clear();
            }
        } else {
            token += std::tolower(static_cast<unsigned char>(c));
        }
    }
    if (!token.empty()) {
        uint64_t hash = 0;
        for (char ch : token) {
            hash = hash * 31 + static_cast<unsigned char>(ch);
        }
        hashes.push_back(hash);
    }
    
    // Calculate SimHash using bit voting
    std::vector<int> bit_counts(64, 0);  // 64-bit hash
    
    for (uint64_t hash : hashes) {
        for (int i = 0; i < 64; i++) {
            if ((hash >> i) & 1) {
                bit_counts[i]++;
            }
        }
    }
    
    // Construct the final hash
    uint64_t simhash = 0;
    for (int i = 0; i < 64; i++) {
        if (bit_counts[i] > (int)(hashes.size() / 2)) {
            simhash |= (1ULL << i);
        }
    }
    
    return simhash;
}

/**
 * Calculate Hamming distance between two hashes
 * Lower distance = more similar content
 */
int WebCrawler::hamming_distance(uint64_t hash1, uint64_t hash2) {
    uint64_t xor_result = hash1 ^ hash2;
    int distance = 0;
    
    while (xor_result) {
        distance += xor_result & 1;
        xor_result >>= 1;
    }
    
    return distance;
}

/**
 * Check if content is a duplicate based on SimHash
 * threshold: maximum hamming distance to consider as duplicate (0-64)
 */
bool WebCrawler::is_duplicate(uint64_t content_hash, int threshold) {
    if (!enable_deduplication_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dedup_mutex_);
    
    // Check against all previously stored hashes
    for (uint64_t stored_hash : content_hashes_) {
        int distance = hamming_distance(content_hash, stored_hash);
        if (distance <= threshold) {
            duplicates_detected_++;
            return true;
        }
    }
    
    // Add this hash to the collection
    content_hashes_.push_back(content_hash);
    return false;
}

/**
 * Set HTTP configuration (HTTP version preferences, SSL settings, etc.)
 */
void WebCrawler::set_http_config(const HTTPConfig& config) {
    http_config_ = config;
    
    if (http_config_.enable_http2) {
        log_info("HTTP/2 support enabled (with HTTP/1.1 fallback)");
    } else {
        log_info("HTTP/1.1 only mode");
    }
}

/**
 * Get current HTTP configuration
 */
HTTPConfig WebCrawler::get_http_config() const {
    return http_config_;
}
