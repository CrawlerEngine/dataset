#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

/**
 * Configuration structure for the crawler
 */
struct CrawlerConfig {
    // Crawler settings
    long timeout;
    int max_retries;
    std::string user_agent;
    bool follow_redirects;
    bool respect_robots_txt;
    bool respect_meta_tags;

    // Output settings
    std::string output_format;  // "json", "csv", "both"
    std::string output_dir;
    int batch_size;

    // URLs to crawl
    std::vector<std::string> urls;

    // Custom headers
    std::map<std::string, std::string> headers;

    // Default constructor
    CrawlerConfig() 
        : timeout(30), max_retries(3), user_agent("DatasetCrawler/1.0"),
          follow_redirects(true), respect_robots_txt(true),
          respect_meta_tags(true), output_format("json"),
          output_dir("./output"), batch_size(1000) {
    }
};

/**
 * Loads configuration from JSON file
 */
class ConfigLoader {
public:
    /**
     * Load configuration from JSON file
     * @param filepath Path to config.json file
     * @return CrawlerConfig object
     * @throws std::runtime_error if file not found or invalid JSON
     */
    static CrawlerConfig load(const std::string& filepath);

    /**
     * Load configuration from command line arguments
     * @param argc Number of arguments
     * @param argv Array of arguments
     * @return CrawlerConfig object with overrides
     */
    static CrawlerConfig from_command_line(int argc, char* argv[]);

    /**
     * Get default configuration
     * @return Default CrawlerConfig
     */
    static CrawlerConfig get_default();

    /**
     * Save configuration to JSON file
     * @param filepath Path to output config file
     * @param config Configuration to save
     */
    static void save(const std::string& filepath, const CrawlerConfig& config);

private:
    static std::string read_file(const std::string& filepath);
    static CrawlerConfig parse_json(const std::string& json_str);
};

#endif // CONFIG_LOADER_H
