#include "crawler.h"
#include "dataset_writer.h"
#include "logger.h"
#include "config_loader.h"
#include "http_config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

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

        // Crawl URLs
        auto records = crawler.crawl_urls(config.urls);

        // Write to files
        ParquetDatasetWriter writer;
        
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

        return 0;

    } catch (const std::exception& e) {
        std::ostringstream error_msg;
        error_msg << "Fatal error: " << e.what();
        log_error(error_msg.str());
        return 1;
    }
}
