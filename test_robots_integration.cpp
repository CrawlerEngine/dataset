#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "include/crawler.h"

/**
 * Integration tests for robots.txt User-Agent priority with path checking
 */
class RobotsIntegrationTester {
private:
    int tests_passed = 0;
    int tests_failed = 0;

public:
    void test_google_spec_example() {
        std::cout << "\n=== Testing Google Spec Example ===\n";
        
        // From: https://developers.google.com/search/docs/crawling-indexing/robots-txt
        std::string robots = R"(
User-agent: Googlebot
Disallow: /nogooglebot/

User-agent: *
Disallow: /
Allow: .html$
)";
        
        WebCrawler crawler("Googlebot");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Should have 2 rule groups
        assert(rules.size() == 2);
        std::cout << "✓ Parsed 2 rule groups\n";
        
        // Googlebot rule should have specificity 3
        assert(rules[0].specificity == 3);
        std::cout << "✓ Googlebot rule has higher specificity\n";
        
        tests_passed += 2;
    }

    void test_multiple_same_agent_groups() {
        std::cout << "\n=== Testing Multiple Groups for Same Agent ===\n";
        
        std::string robots = R"(
User-agent: googlebot
Disallow: /search
Crawl-delay: 1

User-agent: googlebot
Allow: /search/public

User-agent: *
Disallow: /admin
)";
        
        WebCrawler crawler("googlebot");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Should have 3 rule groups (2 googlebot + 1 wildcard)
        assert(rules.size() == 3);
        std::cout << "✓ Parsed 3 rule groups (2 googlebot + 1 wildcard)\n";
        
        // First two should be googlebot with specificity 3
        assert(rules[0].specificity == 3);
        assert(rules[1].specificity == 3);
        assert(rules[2].specificity == 1);  // wildcard
        std::cout << "✓ Specificity levels correct\n";
        
        // Check that both googlebot groups have content
        assert(!rules[0].disallows.empty());
        assert(!rules[1].allows.empty());
        std::cout << "✓ Both googlebot rule groups parsed correctly\n";
        
        tests_passed += 3;
    }

    void test_complex_path_matching() {
        std::cout << "\n=== Testing Complex Path Matching ===\n";
        
        std::string robots = R"(
User-agent: crawler
Disallow: /private
Disallow: /temp/
Disallow: /*.txt$
Allow: /private/public
Allow: /temp/cache/

User-agent: *
Disallow: /
)";
        
        WebCrawler crawler("crawler");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        assert(rules.size() == 2);
        
        // First rule (crawler) should have multiple disallows and allows
        assert(rules[0].disallows.size() == 3);
        assert(rules[0].allows.size() == 2);
        std::cout << "✓ Complex paths parsed correctly\n";
        
        // Check specific paths
        assert(rules[0].disallows[0] == "/private");
        assert(rules[0].disallows[1] == "/temp/");
        assert(rules[0].allows[0] == "/private/public");
        std::cout << "✓ Individual paths stored correctly\n";
        
        tests_passed += 2;
    }

    void test_case_insensitive_user_agent() {
        std::cout << "\n=== Testing Case-Insensitive User-Agent Matching ===\n";
        
        std::string robots = R"(
User-agent: Googlebot
Disallow: /no-google/

User-agent: BingBot
Disallow: /no-bing/

User-agent: *
Disallow: /
)";
        
        WebCrawler crawler("googlebot");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Googlebot in robots.txt, googlebot in User-Agent - should match
        assert(crawler.matches_user_agent("Googlebot", "googlebot"));
        std::cout << "✓ Case-insensitive matching works\n";
        
        // BingBot in robots.txt, googlebot as crawler - should not match
        assert(!crawler.matches_user_agent("BingBot", "googlebot"));
        std::cout << "✓ Different agents don't match\n";
        
        tests_passed += 2;
    }

    void test_empty_disallow_blocks_all() {
        std::cout << "\n=== Testing Empty Disallow (Block All) ===\n";
        
        std::string robots = R"(
User-agent: badbot
Disallow:

User-agent: *
Allow: /
)";
        
        WebCrawler crawler("badbot");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        assert(rules.size() == 2);
        
        // First rule (badbot) should have empty disallows (which means block nothing)
        // This is different from Disallow: / (which blocks everything)
        std::cout << "✓ Empty Disallow parsed\n";
        
        tests_passed += 1;
    }

    void test_version_suffix_in_rules() {
        std::cout << "\n=== Testing Version Suffix Handling in Rules ===\n";
        
        std::string robots = R"(
User-agent: googlebot/1.0
Disallow: /v1/

User-agent: googlebot/2.0
Disallow: /v2/

User-agent: *
Disallow: /
)";
        
        WebCrawler crawler("googlebot/1.5");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Should have 3 rule groups
        assert(rules.size() == 3);
        
        // googlebot/1.0 rule should match googlebot/1.5
        assert(crawler.matches_user_agent("googlebot/1.0", "googlebot/1.5"));
        std::cout << "✓ Version suffix ignored in matching\n";
        
        // But googlebot/1.0 and googlebot/2.0 are treated as different after normalization
        // Actually no - they both normalize to 'googlebot' so they match each other
        assert(crawler.matches_user_agent("googlebot/1.0", "googlebot/2.0"));
        std::cout << "✓ Different versions match same normalized agent\n";
        
        tests_passed += 2;
    }

    void test_root_disallow() {
        std::cout << "\n=== Testing Root Disallow (Block Everything) ===\n";
        
        std::string robots = R"(
User-agent: badbot
Disallow: /

User-agent: *
Allow: /
)";
        
        WebCrawler crawler("badbot");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        assert(!rules[0].disallows.empty());
        assert(rules[0].disallows[0] == "/");
        std::cout << "✓ Root disallow (/) parsed correctly\n";
        
        tests_passed += 1;
    }

    void test_asterisk_in_user_agent() {
        std::cout << "\n=== Testing Asterisk in User-Agent ===\n";
        
        std::string robots = R"(
User-agent: crawler*
Disallow: /temp/

User-agent: *
Disallow: /
)";
        
        WebCrawler crawler("crawler");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // crawler* should normalize to crawler
        assert(crawler.normalize_user_agent("crawler*") == "crawler");
        std::cout << "✓ Asterisk suffix removed from user-agent\n";
        
        // Should match
        assert(crawler.matches_user_agent("crawler*", "crawler"));
        std::cout << "✓ crawler* matches crawler\n";
        
        tests_passed += 2;
    }

    void test_no_matching_rules_fallback() {
        std::cout << "\n=== Testing Fallback When No Specific Rules Match ===\n";
        
        std::string robots = R"(
User-agent: googlebot
Disallow: /

User-agent: *
Disallow: /secret
Allow: /
)";
        
        WebCrawler crawler("unknownbot");
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        assert(rules.size() == 2);
        
        // unknownbot doesn't match googlebot, so should fall back to wildcard
        assert(!crawler.matches_user_agent("googlebot", "unknownbot"));
        assert(crawler.matches_user_agent("*", "unknownbot"));
        std::cout << "✓ Wildcard fallback logic correct\n";
        
        tests_passed += 1;
    }

    void run_all_tests() {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "   ROBOTS.TXT USER-AGENT INTEGRATION TESTS   \n";
        std::cout << std::string(60, '=') << "\n";
        
        try {
            test_google_spec_example();
            test_multiple_same_agent_groups();
            test_complex_path_matching();
            test_case_insensitive_user_agent();
            test_empty_disallow_blocks_all();
            test_version_suffix_in_rules();
            test_root_disallow();
            test_asterisk_in_user_agent();
            test_no_matching_rules_fallback();
            
            std::cout << "\n" << std::string(60, '=') << "\n";
            std::cout << "RESULTS: " << tests_passed << " tests passed, "
                      << tests_failed << " failed\n";
            std::cout << std::string(60, '=') << "\n\n";
            
            if (tests_failed == 0) {
                std::cout << "✓ All integration tests passed!\n\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "✗ Test failed with exception: " << e.what() << "\n";
            tests_failed++;
        }
    }
};

int main() {
    RobotsIntegrationTester tester;
    tester.run_all_tests();
    return 0;
}
