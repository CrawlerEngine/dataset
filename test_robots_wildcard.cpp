#include "crawler.h"
#include <iostream>
#include <cassert>
#include <sstream>

struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
};

std::vector<TestResult> results;

void add_result(const std::string& name, bool passed, const std::string& error = "") {
    results.push_back({name, passed, error});
    if (passed) {
        std::cout << "✓ " << name << std::endl;
    } else {
        std::cout << "✗ " << name << " - " << error << std::endl;
    }
}

int main() {
    WebCrawler crawler("testbot");
    results.clear();

    std::cout << "=== Testing Basic Path Matching ===" << std::endl;
    
    // Test simple prefix matching /fish
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"testbot"};
    rule.disallows = {"/fish"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /fish should match /fish and /fishheads
    add_result("/fish matches /fish", !crawler.is_path_allowed(rules, "/fish"));
    add_result("/fish matches /fishheads", !crawler.is_path_allowed(rules, "/fishheads"));
    add_result("/fish matches /fish.html", !crawler.is_path_allowed(rules, "/fish.html"));
    // Note: /fish also matches /fish/ according to Google spec (prefix matching)
    add_result("/fish also matches /fish/ (prefix)", !crawler.is_path_allowed(rules, "/fish/"));
    add_result("/fish does NOT match /catfish", crawler.is_path_allowed(rules, "/catfish"));
    
    std::cout << "\n=== Testing End-of-URL Marker ($) ===" << std::endl;
    
    rules.clear();
    rule.user_agents = {"testbot"};
    rule.disallows = {"/$"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /$  matches only /
    add_result("/$  matches /", !crawler.is_path_allowed(rules, "/"));
    add_result("/$  does NOT match /test", crawler.is_path_allowed(rules, "/test"));
    add_result("/$  does NOT match /test/", crawler.is_path_allowed(rules, "/test/"));
    
    std::cout << "\n=== Testing Wildcard (*) in Patterns ===" << std::endl;
    
    rules.clear();
    rule.user_agents = {"testbot"};
    rule.disallows = {"/*.php"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /*.php matches any .php file
    add_result("/*.php matches /index.php", !crawler.is_path_allowed(rules, "/index.php"));
    add_result("/*.php matches /dir/index.php", !crawler.is_path_allowed(rules, "/dir/index.php"));
    add_result("/*.php matches /dir/file.php", !crawler.is_path_allowed(rules, "/dir/file.php"));
    add_result("/*.php does NOT match /file.html", crawler.is_path_allowed(rules, "/file.html"));
    add_result("/*.php does NOT match /php", crawler.is_path_allowed(rules, "/php"));
    
    std::cout << "\n=== Testing Wildcard with End Marker (*.php$) ===" << std::endl;
    
    rules.clear();
    rule.user_agents = {"testbot"};
    rule.disallows = {"/*.php$"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /*.php$ matches .php files at the end
    add_result("/*.php$ matches /index.php", !crawler.is_path_allowed(rules, "/index.php"));
    add_result("/*.php$ matches /dir/test.php", !crawler.is_path_allowed(rules, "/dir/test.php"));
    add_result("/*.php$ does NOT match /index.php.bak", crawler.is_path_allowed(rules, "/index.php.bak"));
    add_result("/*.php$ does NOT match /file.html", crawler.is_path_allowed(rules, "/file.html"));
    
    std::cout << "\n=== Testing Complex Patterns ===" << std::endl;
    
    rules.clear();
    rule.user_agents = {"testbot"};
    rule.disallows = {"/fish*.php"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /fish*.php matches patterns like /fish1.php, /fishtest.php
    add_result("/fish*.php matches /fish.php", !crawler.is_path_allowed(rules, "/fish.php"));
    add_result("/fish*.php matches /fish123.php", !crawler.is_path_allowed(rules, "/fish123.php"));
    add_result("/fish*.php matches /fishheads.php", !crawler.is_path_allowed(rules, "/fishheads.php"));
    add_result("/fish*.php does NOT match /catfish.php", crawler.is_path_allowed(rules, "/catfish.php"));
    add_result("/fish*.php does NOT match /fish.html", crawler.is_path_allowed(rules, "/fish.html"));
    
    std::cout << "\n=== Testing Longest Match Wins ===" << std::endl;
    
    rules.clear();
    
    // Rule 1: disallow /
    rule.user_agents = {"testbot"};
    rule.disallows = {"/"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // Rule 2: allow /admin
    rule.user_agents = {"testbot"};
    rule.disallows = {};
    rule.allows = {"/admin"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    add_result("Longest match /admin allowed when both / and /admin apply", 
               crawler.is_path_allowed(rules, "/admin"));
    add_result("Disallow / when no longer allow applies", 
               !crawler.is_path_allowed(rules, "/other"));
    
    std::cout << "\n=== Testing Allow Wins on Equal Length ===" << std::endl;
    
    rules.clear();
    
    // Rule with same length allow and disallow
    rule.user_agents = {"testbot"};
    rule.disallows = {"/test"};
    rule.allows = {"/test"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    add_result("Allow wins over Disallow when equal length", 
               crawler.is_path_allowed(rules, "/test"));
    
    std::cout << "\n=== Testing Directory Paths ===" << std::endl;
    
    rules.clear();
    rule.user_agents = {"testbot"};
    rule.disallows = {"/fish/"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /fish/ only blocks paths within /fish/ directory
    add_result("/fish/ matches /fish/salmon.html", !crawler.is_path_allowed(rules, "/fish/salmon.html"));
    add_result("/fish/ does NOT match /fish", crawler.is_path_allowed(rules, "/fish"));
    add_result("/fish/ does NOT match /fish.html", crawler.is_path_allowed(rules, "/fish.html"));
    
    std::cout << "\n=== Testing Multiple Wildcards ===" << std::endl;
    
    rules.clear();
    rule.user_agents = {"testbot"};
    rule.disallows = {"/*.php*"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    add_result("/*.php* matches /index.php", !crawler.is_path_allowed(rules, "/index.php"));
    add_result("/*.php* matches /index.php.bak", !crawler.is_path_allowed(rules, "/index.php.bak"));
    add_result("/*.php* matches /file.php5", !crawler.is_path_allowed(rules, "/file.php5"));
    
    std::cout << "\n=== Testing Google Examples from Specification ===" << std::endl;
    
    // Example from Google spec: /fish should match /fish, /fish.html, /fishheads but not /catfish
    rules.clear();
    rule.user_agents = {"*"};
    rule.disallows = {"/fish"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    add_result("[Google] /fish matches /fish", !crawler.is_path_allowed(rules, "/fish"));
    add_result("[Google] /fish matches /fish.html", !crawler.is_path_allowed(rules, "/fish.html"));
    add_result("[Google] /fish matches /fishheads", !crawler.is_path_allowed(rules, "/fishheads"));
    add_result("[Google] /fish does NOT match /catfish", crawler.is_path_allowed(rules, "/catfish"));
    
    // Example: /*.php should match any .php file at any level
    rules.clear();
    rule.user_agents = {"*"};
    rule.disallows = {"/*.php"};
    rule.allows = {};
    rule.specificity = 1;
    rules.push_back(rule);
    
    add_result("[Google] /*.php matches /index.php", !crawler.is_path_allowed(rules, "/index.php"));
    add_result("[Google] /*.php matches /dir/file.php", !crawler.is_path_allowed(rules, "/dir/file.php"));
    
    // Print summary
    std::cout << "\n==================================================";
    int passed = 0, failed = 0;
    for (const auto& result : results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
        }
    }
    std::cout << "\nRESULTS: " << passed << " tests passed, " << failed << " failed";
    std::cout << "\n==================================================\n";
    
    if (failed == 0) {
        std::cout << "✓ All wildcard tests passed!\n";
        return 0;
    } else {
        std::cout << "✗ Some tests failed\n";
        return 1;
    }
}
