#include "crawler.h"
#include "dataset_writer.h"
#include <iostream>
#include <vector>
#include <thread>

/**
 * Example 1: Simple crawling with single requests
 */
void example_simple_crawl() {
    std::cout << "\n=== Example 1: Simple Single URL Crawl ===" << std::endl;
    
    WebCrawler crawler;
    crawler.set_timeout(30);
    
    DataRecord record = crawler.fetch("https://example.com");
    
    std::cout << "URL: " << record.url << std::endl;
    std::cout << "Status: " << record.status_code << std::endl;
    std::cout << "Title: " << record.title << std::endl;
    std::cout << "Content length: " << record.content.size() << " bytes" << std::endl;
}

/**
 * Example 2: Batch crawling multiple URLs
 */
void example_batch_crawl() {
    std::cout << "\n=== Example 2: Batch URL Crawl ===" << std::endl;
    
    WebCrawler crawler;
    crawler.set_timeout(20);
    crawler.add_header("Accept-Language", "en-US,en;q=0.9");
    
    std::vector<std::string> urls = {
        "https://example.com",
        "https://example.org",
    };
    
    auto records = crawler.crawl_urls(urls);
    
    // Save to Parquet
    ParquetDatasetWriter writer;
    writer.write_records("batch_dataset.parquet", records);
}

/**
 * Example 3: Custom headers and authentication
 */
void example_custom_headers() {
    std::cout << "\n=== Example 3: Custom Headers ===" << std::endl;
    
    WebCrawler crawler;
    crawler.set_timeout(30);
    
    // Add custom headers for authentication
    crawler.add_header("Authorization", "Bearer YOUR_TOKEN_HERE");
    crawler.add_header("X-API-Key", "YOUR_API_KEY");
    
    DataRecord record = crawler.fetch("https://api.example.com/data");
    std::cout << "Fetched from API with custom headers" << std::endl;
}

/**
 * Example 4: Save as both Parquet and CSV
 */
void example_multiple_formats() {
    std::cout << "\n=== Example 4: Save as Multiple Formats ===" << std::endl;
    
    WebCrawler crawler;
    
    std::vector<std::string> urls = {
        "https://example.com",
    };
    
    auto records = crawler.crawl_urls(urls);
    
    // Save both formats
    ParquetDatasetWriter writer;
    writer.write_records("dataset_output.parquet", records);
    writer.write_csv("dataset_output.csv", records);
    
    std::cout << "Data saved in both Parquet and CSV formats" << std::endl;
}

/**
 * Example 5: Error handling
 */
void example_error_handling() {
    std::cout << "\n=== Example 5: Error Handling ===" << std::endl;
    
    WebCrawler crawler;
    crawler.set_timeout(5); // Short timeout for testing
    
    try {
        // Try to fetch from an invalid URL
        DataRecord record = crawler.fetch("https://invalid-url-that-does-not-exist-12345.com");
        
        if (record.status_code == 0) {
            std::cout << "Connection error or timeout occurred" << std::endl;
        } else if (record.status_code != 200) {
            std::cout << "HTTP Error " << record.status_code << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }
}

/**
 * Example 6: Incremental data collection
 */
void example_incremental_collection() {
    std::cout << "\n=== Example 6: Incremental Data Collection ===" << std::endl;
    
    WebCrawler crawler;
    ParquetDatasetWriter writer;
    
    // First batch
    std::vector<std::string> batch1 = {"https://example.com"};
    auto records1 = crawler.crawl_urls(batch1);
    writer.write_records("incremental_dataset.parquet", records1);
    
    // Second batch - append
    std::vector<std::string> batch2 = {"https://example.org"};
    auto records2 = crawler.crawl_urls(batch2);
    writer.append_records("incremental_dataset.parquet", records2);
    
    std::cout << "Created incremental dataset with appending" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Dataset Crawler Examples ===" << std::endl;
    std::cout << "C++ Web Crawler with Parquet Export" << std::endl;
    
    if (argc > 1) {
        int example = std::stoi(argv[1]);
        
        switch (example) {
            case 1:
                example_simple_crawl();
                break;
            case 2:
                example_batch_crawl();
                break;
            case 3:
                example_custom_headers();
                break;
            case 4:
                example_multiple_formats();
                break;
            case 5:
                example_error_handling();
                break;
            case 6:
                example_incremental_collection();
                break;
            default:
                std::cout << "Usage: " << argv[0] << " <example_number>" << std::endl;
                std::cout << "  1 - Simple crawl" << std::endl;
                std::cout << "  2 - Batch crawl" << std::endl;
                std::cout << "  3 - Custom headers" << std::endl;
                std::cout << "  4 - Multiple formats" << std::endl;
                std::cout << "  5 - Error handling" << std::endl;
                std::cout << "  6 - Incremental collection" << std::endl;
        }
    } else {
        // Run all examples
        try {
            example_simple_crawl();
            example_batch_crawl();
            example_multiple_formats();
        } catch (const std::exception& e) {
            std::cerr << "Error in examples: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
