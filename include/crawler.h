#ifndef CRAWLER_H
#define CRAWLER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include "http_config.h"
#include "rocksdb_manager.h"
#include "text_extractor.h"

/**
 * robots.txt rules for a specific user-agent group
 */
struct RobotRule {
    std::vector<std::string> user_agents;
    std::vector<std::string> disallows;
    std::vector<std::string> allows;
    int specificity = 0;  // Higher = more specific (exact match > pattern > wildcard)
    double crawl_delay_seconds = -1.0;
};

struct DataRecord {
    std::string url;
    std::string title;
    std::string content;
    std::string timestamp;
    int status_code;
    bool was_allowed;
    size_t content_length;  // Size of downloaded content
    bool was_skipped;       // Whether skipped due to size limit
};

/**
 * Crawling statistics
 */
struct CrawlerStats {
    int total_requests = 0;
    int successful_requests = 0;
    int failed_requests = 0;
    int blocked_by_robots = 0;
    int blocked_by_noindex = 0;
    int skipped_by_size = 0;
    int sitemaps_found = 0;
    int duplicates_detected = 0;  // Number of duplicates found by SimHash
    int http2_requests = 0;        // Number of requests using HTTP/2
    int http11_requests = 0;       // Number of requests using HTTP/1.1
    int http10_requests = 0;       // Number of requests using HTTP/1.0
    long total_bytes_downloaded = 0;
    long total_duration_ms = 0;
    double avg_request_duration_ms = 0.0;
    double requests_per_minute = 0.0;
};

class WebCrawler {
public:
    WebCrawler(const std::string& user_agent = "DatasetCrawler/1.0");
    ~WebCrawler();

    /**
     * Fetch content from a single URL
     */
    DataRecord fetch(const std::string& url);

    /**
     * Crawl multiple URLs
     */
    std::vector<DataRecord> crawl_urls(const std::vector<std::string>& urls);

    /**
     * Set connection timeout (in seconds)
     */
    void set_timeout(long timeout_seconds);

    /**
     * Set custom headers
     */
    void add_header(const std::string& key, const std::string& value);

    /**
     * Enable/disable robots.txt and meta-tag checking
     */
    void set_respect_robots_txt(bool respect);
    void set_respect_meta_tags(bool respect);

    /**
     * Set max file size (in MB). Files larger than this will be skipped
     */
    void set_max_file_size(size_t size_mb);

    /**
     * Get statistics about blocked pages
     */
    int get_blocked_by_robots_count() const;
    int get_blocked_by_noindex_count() const;
    int get_skipped_by_size_count() const;
    int get_sitemaps_found_count() const;

    /**
     * Enable/disable periodic statistics reporting (reports every minute)
     */
    void enable_periodic_stats(bool enable);
    bool is_periodic_stats_enabled() const;

    /**
     * Get detailed crawling statistics
     */
    CrawlerStats get_statistics() const;

    /**
     * Parse sitemaps from robots.txt
     */
    std::vector<std::string> get_sitemaps_from_robots(const std::string& domain);

    /**
     * Fetch and parse sitemap.xml
     */
    std::vector<std::string> fetch_sitemap_urls(const std::string& sitemap_url);

    /**
     * Public methods for testing robots.txt User-Agent matching
     */
    bool matches_user_agent(const std::string& rule_agent, const std::string& crawler_agent) const;
    std::string normalize_user_agent(const std::string& agent) const;
    std::vector<RobotRule> parse_robots_txt(const std::string& host, const std::string& robots_content);

    /**
     * Public methods for testing robots.txt path matching with wildcards
     */
    bool is_path_allowed(const std::vector<RobotRule>& rules, const std::string& path);
    bool is_path_allowed(const std::vector<RobotRule>& rules, const std::string& path, const std::string& user_agent);
    bool match_path_pattern(const std::string& pattern, const std::string& path);

    /**
     * Deduplication methods using SimHash
     */
    void enable_deduplication(bool enable);
    bool is_deduplication_enabled() const;
    uint64_t calculate_simhash(const std::string& content);
    int hamming_distance(uint64_t hash1, uint64_t hash2);
    bool is_duplicate(uint64_t content_hash, int threshold = 3);
    int get_duplicates_detected_count() const;

    /**
     * HTTP configuration and protocol support
     */
    void set_http_config(const HTTPConfig& config);
    HTTPConfig get_http_config() const;

private:
    std::string user_agent_;
    HTTPConfig http_config_;  // HTTP/1.1, HTTP/2 configuration
    long timeout_;
    std::map<std::string, std::string> headers_;
    bool respect_robots_txt_;
    bool respect_meta_tags_;
    size_t max_file_size_bytes_;  // Maximum file size to crawl
    
    // RocksDB and Text Extraction
    std::unique_ptr<RocksDBManager> db_manager_;
    std::unique_ptr<TextExtractor> text_extractor_;
    std::string db_path_;
    
    // Statistics
    int blocked_by_robots_;
    int blocked_by_noindex_;
    int skipped_by_size_;
    int sitemaps_found_;
    int duplicates_detected_;
    int http2_requests_;
    int http11_requests_;
    int http10_requests_;
    long total_bytes_downloaded_;
    long total_duration_ms_;
    std::vector<long> request_durations_;  // For calculating avg
    long last_request_duration_ms_;
    double latency_ema_ms_;
    int consecutive_failures_;
    int consecutive_successes_;
    int last_delay_ms_;
    std::chrono::steady_clock::time_point last_request_time_;
    std::string current_domain_;
    
    std::map<std::string, bool> robots_cache_;
    std::map<std::string, std::vector<std::string>> robots_sitemaps_cache_;
    std::map<std::string, std::vector<RobotRule>> robots_rules_cache_;  // Cache parsed robots.txt rules
    std::map<std::string, double> robots_crawl_delay_cache_;
    std::map<std::string, std::chrono::steady_clock::time_point> robots_cache_time_;
    std::map<std::string, std::chrono::steady_clock::time_point> robots_sitemaps_cache_time_;
    std::chrono::steady_clock::time_point crawl_start_time_;
    
    // Memory-efficient caches using STL hash containers
    std::unordered_set<std::string> visited_urls_memory_;  // In-memory cache for fast lookup
    
    // Periodic statistics reporting
    bool enable_periodic_stats_;
    std::atomic<bool> stats_thread_running_;
    std::thread stats_reporter_thread_;
    mutable std::mutex stats_mutex_;
    
    // Deduplication
    bool enable_deduplication_;
    std::vector<uint64_t> content_hashes_;  // Store SimHashes for deduplication
    std::mutex dedup_mutex_;
    
    void start_stats_reporter();
    void stop_stats_reporter();
    void stats_reporter_loop();
    std::string format_stats_message(const CrawlerStats& stats);

    std::string fetch_html(const std::string& url, int& status_code);
    std::string extract_title(const std::string& html);
    bool check_robots_txt(const std::string& url);
    bool check_meta_tags(const std::string& html);
    std::string get_domain(const std::string& url);
    std::vector<std::string> extract_sitemap_urls_from_robots(const std::string& robots_content);
    std::vector<std::string> parse_sitemap_xml(const std::string& xml_content);
    
    // Link extraction and normalization
    std::vector<std::string> extract_links_from_html(const std::string& html, const std::string& base_url);
    std::string normalize_url(const std::string& url);
    std::string resolve_relative_url(const std::string& base_url, const std::string& relative_url);
    std::string extract_canonical_url(const std::string& html, const std::string& base_url);
    bool is_valid_url(const std::string& url);
    
    // Encoding detection and conversion
    std::string detect_encoding(const std::string& content, const std::string& content_type);
    std::string convert_to_utf8(const std::string& content, const std::string& from_encoding);
    void apply_adaptive_delay(int status_code);
    double get_crawl_delay_for_domain(const std::string& domain) const;
    std::vector<std::string> parse_sitemap_index_xml(const std::string& xml_content);
};

#endif
