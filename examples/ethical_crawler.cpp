// Example: Ethical Web Crawling with robots.txt and meta-tag support
// Compile with: g++ -std=c++17 -o ethical_crawler ethical_crawler.cpp \
//               -lcurl -I../include ../src/crawler.cpp ../src/dataset_writer.cpp

#include "crawler.h"
#include "dataset_writer.h"
#include <iostream>
#include <vector>

/**
 * Example 1: Basic ethical crawling (respects robots.txt and meta-tags)
 */
void example_ethical_crawling() {
    std::cout << "\n=== Example 1: Ethical Crawling ===" << std::endl;
    
    WebCrawler crawler;
    crawler.set_timeout(30);
    
    // These are enabled by default
    crawler.set_respect_robots_txt(true);
    crawler.set_respect_meta_tags(true);
    
    std::vector<std::string> urls = {
        "https://example.com",
        "https://example.com/page1",
        "https://example.com/page2",
    };
    
    auto records = crawler.crawl_urls(urls);
    
    // Get statistics about blocked pages
    int blocked_robots = crawler.get_blocked_by_robots_count();
    int blocked_noindex = crawler.get_blocked_by_noindex_count();
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Collected: " << records.size() << " pages" << std::endl;
    std::cout << "  Blocked by robots.txt: " << blocked_robots << std::endl;
    std::cout << "  Blocked by noindex: " << blocked_noindex << std::endl;
}

/**
 * Example 2: Reporting on compliance
 */
void example_compliance_report() {
    std::cout << "\n=== Example 2: Compliance Report ===" << std::endl;
    
    WebCrawler crawler;
    crawler.set_respect_robots_txt(true);
    crawler.set_respect_meta_tags(true);
    
    std::vector<std::string> urls = {
        "https://example.com",
        "https://example.org",
    };
    
    auto records = crawler.crawl_urls(urls);
    
    int total = urls.size();
    int collected = records.size();
    int blocked_robots = crawler.get_blocked_by_robots_count();
    int blocked_noindex = crawler.get_blocked_by_noindex_count();
    int blocked_total = blocked_robots + blocked_noindex;
    
    std::cout << "\n--- Crawling Compliance Report ---" << std::endl;
    std::cout << "Total URLs attempted: " << total << std::endl;
    std::cout << "Successfully crawled: " << collected << std::endl;
    std::cout << "Total blocked: " << blocked_total << std::endl;
    
    if (blocked_robots > 0) {
        std::cout << "  - By robots.txt: " << blocked_robots << std::endl;
    }
    if (blocked_noindex > 0) {
        std::cout << "  - By meta noindex: " << blocked_noindex << std::endl;
    }
    
    if (total > 0) {
        double compliance_rate = (collected * 100.0) / total;
        std::cout << "\nCompliance rate: " << compliance_rate << "%" << std::endl;
    }
}

/**
 * Example 3: Disabling checks (for testing only)
 */
void example_without_restrictions() {
    std::cout << "\n=== Example 3: Without Restrictions (Testing Only) ===" << std::endl;
    
    WebCrawler crawler;
    
    // Disable ethical checks - USE WITH CAUTION!
    crawler.set_respect_robots_txt(false);
    crawler.set_respect_meta_tags(false);
    
    std::cout << "WARNING: Crawling without respecting robots.txt and meta-tags!" << std::endl;
    
    std::vector<std::string> urls = {
        "https://example.com",
    };
    
    auto records = crawler.crawl_urls(urls);
    
    std::cout << "Crawled: " << records.size() << " pages" << std::endl;
}

/**
 * Example 4: Custom User-Agent for identification
 */
void example_custom_user_agent() {
    std::cout << "\n=== Example 4: Custom User-Agent ===" << std::endl;
    
    // Properly identify your crawler
    WebCrawler crawler("MyDatasetBot/1.0 (+https://example.com/bot)");
    
    std::cout << "Using custom User-Agent for identification" << std::endl;
    std::cout << "This helps website owners identify your crawler" << std::endl;
    
    std::vector<std::string> urls = {
        "https://example.com",
    };
    
    auto records = crawler.crawl_urls(urls);
    std::cout << "Crawled: " << records.size() << " pages" << std::endl;
}

/**
 * Example 5: Batch crawling with ethics
 */
void example_batch_crawling() {
    std::cout << "\n=== Example 5: Batch Crawling with Ethics ===" << std::endl;
    
    WebCrawler crawler("DatasetBot/1.0");
    crawler.set_timeout(20);
    crawler.set_respect_robots_txt(true);
    crawler.set_respect_meta_tags(true);
    
    // Crawl in batches
    std::vector<std::vector<std::string>> batches = {
        {
            "https://wikipedia.org/wiki/Machine_learning",
            "https://wikipedia.org/wiki/Artificial_intelligence",
        },
        {
            "https://wikipedia.org/wiki/Data_science",
            "https://wikipedia.org/wiki/Neural_network",
        }
    };
    
    int batch_num = 0;
    std::vector<DataRecord> all_records;
    
    for (const auto& batch : batches) {
        batch_num++;
        std::cout << "\nCrawling batch " << batch_num << "..." << std::endl;
        
        auto records = crawler.crawl_urls(batch);
        all_records.insert(all_records.end(), records.begin(), records.end());
        
        // Print batch statistics
        int blocked_robots = crawler.get_blocked_by_robots_count();
        int blocked_noindex = crawler.get_blocked_by_noindex_count();
        
        std::cout << "  Collected: " << records.size() << std::endl;
        std::cout << "  Blocked (robots): " << blocked_robots << std::endl;
        std::cout << "  Blocked (noindex): " << blocked_noindex << std::endl;
    }
    
    // Save all records
    ParquetDatasetWriter writer;
    writer.write_records("ethical_dataset.json", all_records);
    writer.write_csv("ethical_dataset.csv", all_records);
    
    std::cout << "\nTotal collected: " << all_records.size() << " pages" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Ethical Web Crawling Examples ===" << std::endl;
    
    if (argc > 1) {
        int example = std::stoi(argv[1]);
        
        switch (example) {
            case 1:
                example_ethical_crawling();
                break;
            case 2:
                example_compliance_report();
                break;
            case 3:
                example_without_restrictions();
                break;
            case 4:
                example_custom_user_agent();
                break;
            case 5:
                example_batch_crawling();
                break;
            default:
                std::cout << "Usage: " << argv[0] << " <example_number>" << std::endl;
                std::cout << "  1 - Basic ethical crawling" << std::endl;
                std::cout << "  2 - Compliance report" << std::endl;
                std::cout << "  3 - Without restrictions (testing)" << std::endl;
                std::cout << "  4 - Custom User-Agent" << std::endl;
                std::cout << "  5 - Batch crawling with ethics" << std::endl;
        }
    } else {
        // Run basic example
        example_ethical_crawling();
    }
    
    return 0;
}
