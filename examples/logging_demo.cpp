// Advanced logging examples
// Demonstrates the new structured logging system

#include "logger.h"
#include <iostream>

int main() {
    // Initialize logger
    Logger& log = Logger::instance();
    log.set_color_output(true);
    log.set_level(LogLevel::DEBUG);

    std::cout << "=== Structured Logging Examples ===" << std::endl << std::endl;

    // INFO logs
    log.info("Crawling will be started using 110 start URLs and 0 sitemap URLs");
    log.info("Starting the crawler.");
    log.info("https://example.com [200]");
    log.info("Enqueued 18 new links on https://mathworld.wolfram.com/.");

    std::cout << std::endl;

    // WARN logs
    log.warn("Failed to fetch robots.txt for request https://nlab-pages.org/nlab/show/HomePage", "WCCAdaptiveCrawler");
    log.warn("Reclaiming failed request back to the list or queue. page.goto: NS_ERROR_PROXY_CONNECTION_REFUSED");
    log.warn("Reclaiming failed request back to the list or queue. Detected a session error, rotating session...");

    std::cout << std::endl;

    // ERROR logs
    log.error("Connection timeout after 30 seconds", "WebCrawler");
    log.error("Invalid URL format: 'not a valid url'");

    std::cout << std::endl;

    // DEBUG logs
    log.debug("Cache hit for domain: example.com");
    log.debug("robots.txt parsed successfully for domain: wikipedia.org", "Parser");

    std::cout << std::endl;

    // Colored output demonstration
    std::cout << "Color scheme:" << std::endl;
    log.info("INFO messages are displayed in GREEN");
    log.warn("WARN messages are displayed in YELLOW");
    log.error("ERROR messages are displayed in RED");
    log.debug("DEBUG messages are displayed in CYAN");

    return 0;
}
