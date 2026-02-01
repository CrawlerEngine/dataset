#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "include/crawler.h"

// Test cases for robots.txt User-Agent priority matching
class RobotsTester {
private:
    WebCrawler crawler;
    int tests_passed = 0;
    int tests_failed = 0;

public:
    RobotsTester() : crawler("TestBot/1.0") {}

    void test_normalize_user_agent() {
        std::cout << "\n=== Testing User-Agent Normalization ===\n";
        
        // Create a temporary crawler for testing
        WebCrawler test_crawler("TestBot/1.0");
        
        // Test version suffix removal
        assert(test_crawler.normalize_user_agent("googlebot/1.2") == "googlebot");
        std::cout << "✓ googlebot/1.2 -> googlebot\n";
        
        // Test asterisk removal
        assert(test_crawler.normalize_user_agent("bingbot*") == "bingbot");
        std::cout << "✓ bingbot* -> bingbot\n";
        
        // Test no change needed
        assert(test_crawler.normalize_user_agent("crawler") == "crawler");
        std::cout << "✓ crawler -> crawler\n";
        
        // Test slash removal
        assert(test_crawler.normalize_user_agent("test/agent/1.0") == "test");
        std::cout << "✓ test/agent/1.0 -> test\n";
        
        tests_passed += 4;
    }

    void test_user_agent_matching() {
        std::cout << "\n=== Testing User-Agent Matching ===\n";
        
        WebCrawler test_crawler("TestBot/1.0");
        
        // Exact match
        assert(test_crawler.matches_user_agent("googlebot", "googlebot"));
        std::cout << "✓ Exact match: googlebot == googlebot\n";
        
        // Wildcard match
        assert(test_crawler.matches_user_agent("*", "any-bot"));
        std::cout << "✓ Wildcard match: * matches any-bot\n";
        
        // Version suffix handling
        assert(test_crawler.matches_user_agent("googlebot/1.2", "googlebot/2.0"));
        std::cout << "✓ Version handling: googlebot/1.2 matches googlebot/2.0\n";
        
        // Case-insensitive partial match
        assert(test_crawler.matches_user_agent("GoogleBot", "googlebot"));
        std::cout << "✓ Case-insensitive: GoogleBot matches googlebot\n";
        
        // Non-matching
        assert(!test_crawler.matches_user_agent("googlebot", "bingbot"));
        std::cout << "✓ Non-matching: googlebot != bingbot\n";
        
        tests_passed += 5;
    }

    void test_exact_match_priority() {
        std::cout << "\n=== Testing Exact Match Priority ===\n";
        
        std::string robots = R"(
User-agent: googlebot
Disallow: /admin

User-agent: *
Disallow: /
)";
        
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Should have 2 rule groups
        assert(rules.size() == 2);
        std::cout << "✓ Parsed 2 rule groups\n";
        
        // First rule: googlebot with specificity 3 (exact)
        assert(rules[0].specificity == 3);
        std::cout << "✓ Googlebot rule has specificity 3 (exact match)\n";
        
        // Second rule: wildcard with specificity 1
        assert(rules[1].specificity == 1);
        std::cout << "✓ Wildcard rule has specificity 1\n";
        
        tests_passed += 3;
    }

    void test_allow_precedence() {
        std::cout << "\n=== Testing Allow Precedence Over Disallow ===\n";
        
        std::string robots = R"(
User-agent: testbot
Disallow: /private
Allow: /private/public
)";
        
        auto rules = crawler.parse_robots_txt("example.com", robots);
        assert(!rules.empty());
        
        // Check that Allow is stored
        assert(!rules[0].allows.empty());
        std::cout << "✓ Allow directive parsed\n";
        
        // Allow path should be in allows list
        assert(rules[0].allows[0] == "/private/public");
        std::cout << "✓ /private/public in allows list\n";
        
        // Disallow path should be in disallows list
        assert(rules[0].disallows[0] == "/private");
        std::cout << "✓ /private in disallows list\n";
        
        tests_passed += 3;
    }

    void test_multiple_rules_combination() {
        std::cout << "\n=== Testing Multiple Rule Groups for Same Agent ===\n";
        
        std::string robots = R"(
User-agent: testbot
Disallow: /private

User-agent: testbot
Allow: /private/public

User-agent: *
Disallow: /
)";
        
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Should have 3 rules (2 for testbot, 1 for *)
        assert(rules.size() == 3);
        std::cout << "✓ Parsed 3 rule groups\n";
        
        // First testbot rule should be specific
        assert(rules[0].specificity == 3);
        std::cout << "✓ First testbot rule has specificity 3\n";
        
        // Second testbot rule should also be specific
        assert(rules[1].specificity == 3);
        std::cout << "✓ Second testbot rule has specificity 3\n";
        
        tests_passed += 3;
    }

    void test_wildcard_fallback() {
        std::cout << "\n=== Testing Wildcard Rule Fallback ===\n";
        
        std::string robots = R"(
User-agent: googlebot
Disallow: /admin

User-agent: *
Disallow: /secret
)";
        
        auto rules = crawler.parse_robots_txt("example.com", robots);
        
        // Googlebot: specific rule with /admin
        assert(rules[0].disallows[0] == "/admin");
        std::cout << "✓ Googlebot disallows /admin\n";
        
        // Wildcard: rule with /secret
        assert(rules[1].disallows[0] == "/secret");
        std::cout << "✓ Wildcard disallows /secret\n";
        
        tests_passed += 2;
    }

    void test_empty_disallow() {
        std::cout << "\n=== Testing Empty/Root Disallow ===\n";
        
        std::string robots = R"(
User-agent: badbot
Disallow: /
)";
        
        auto rules = crawler.parse_robots_txt("example.com", robots);
        assert(!rules.empty());
        
        // Should have disallow for root
        assert(!rules[0].disallows.empty());
        std::cout << "✓ Root disallow parsed\n";
        
        tests_passed += 1;
    }

    void run_all_tests() {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "     ROBOTS.TXT USER-AGENT PRIORITY TESTS     \n";
        std::cout << std::string(50, '=') << "\n";
        
        try {
            test_normalize_user_agent();
            test_user_agent_matching();
            test_exact_match_priority();
            test_allow_precedence();
            test_multiple_rules_combination();
            test_wildcard_fallback();
            test_empty_disallow();
            
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "RESULTS: " << tests_passed << " tests passed, "
                      << tests_failed << " failed\n";
            std::cout << std::string(50, '=') << "\n\n";
            
            if (tests_failed == 0) {
                std::cout << "✓ All tests passed!\n\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "✗ Test failed with exception: " << e.what() << "\n";
            tests_failed++;
        }
    }
};

int main() {
    RobotsTester tester;
    tester.run_all_tests();
    return 0;
}
