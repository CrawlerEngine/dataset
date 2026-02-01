// Example: Advanced logging with different scenarios
// Compile with: g++ -std=c++17 -o advanced_logs advanced_logs.cpp \
//               -I../include ../src/logger.cpp

#include "logger.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    Logger& log = Logger::instance();
    log.set_color_output(true);
    log.set_level(LogLevel::DEBUG);

    std::cout << "\n=== Advanced Logging Scenarios ===" << std::endl << std::endl;

    // Scenario 1: Successful crawl
    std::cout << "--- Scenario 1: Successful Crawl ---" << std::endl;
    log.info("Crawling will be started using 110 start URLs and 0 sitemap URLs");
    log.info("Starting the crawler.");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    log.info("https://example.com [200]");
    log.info("https://example.com/page1 [200]");
    log.info("https://example.com/page2 [200]");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    log.info("Enqueued 18 new links on https://mathworld.wolfram.com/.");
    log.info("Enqueued 100 new links on https://www.nature.com/.");
    log.info("Enqueued 75 new links on https://plos.org/.");

    std::cout << "\n--- Scenario 2: Robots.txt Warnings ---" << std::endl;
    log.warn("Failed to fetch robots.txt for request https://nlab-pages.org/nlab/show/HomePage", 
             "WCCAdaptiveCrawler");
    log.info("https://nlab-pages.org/nlab/show/HomePage [403]");

    std::cout << "\n--- Scenario 3: Connection Errors ---" << std::endl;
    log.warn("Reclaiming failed request back to the list or queue. page.goto: NS_ERROR_PROXY_CONNECTION_REFUSED");
    log.error("Connection timeout after 30 seconds", "WebCrawler");

    std::cout << "\n--- Scenario 4: Rate Limiting ---" << std::endl;
    log.warn("Received blocked status code: 429", "RateLimiter");
    log.warn("Reclaiming failed request back to the list or queue. Detected a session error, rotating session...");
    
    std::cout << "\n--- Scenario 5: Success Summary ---" << std::endl;
    log.info("Successfully crawled 45 pages from 3 domains");
    log.info("Successfully wrote 45 records to dataset.json");
    log.info("Successfully wrote 45 records to dataset.csv");

    std::cout << "\n\n=== Color Legend ===" << std::endl;
    log.info("INFO messages - Normal operation information (GREEN)");
    log.warn("WARN messages - Potential issues that don't stop execution (YELLOW)");
    log.error("ERROR messages - Critical issues that may stop execution (RED)");
    log.debug("DEBUG messages - Detailed diagnostic information (CYAN)");

    return 0;
}
