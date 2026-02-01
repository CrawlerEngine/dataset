#include <iostream>
#include <vector>
#include "include/crawler.h"

/**
 * Example: robots.txt User-Agent Priority Matching
 * 
 * This example demonstrates the new robots.txt parsing with proper
 * User-Agent priority matching according to Google's specification.
 */

int main() {
    // Create crawler with specific User-Agent
    WebCrawler crawler("MyBot/1.0");
    
    // Sample robots.txt content
    std::string robots_txt = R"(
# Example robots.txt with multiple User-Agent rules

# Specific rules for Googlebot
User-agent: googlebot
Disallow: /admin
Disallow: /private
Allow: /private/public

# Rules for Bingbot
User-agent: bingbot
Disallow: /temp
Allow: /temp/cache

# Rules for any other bot (wildcard)
User-agent: *
Disallow: /secret
Disallow: /internal
Allow: /internal/docs

# Rules for slower bots
User-agent: slowbot
Crawl-delay: 10
Disallow: /
)";
    
    std::cout << "=== ROBOTS.TXT USER-AGENT PRIORITY EXAMPLE ===\n\n";
    
    // Parse robots.txt
    std::vector<RobotRule> rules = crawler.parse_robots_txt("example.com", robots_txt);
    
    std::cout << "Parsed " << rules.size() << " rule groups:\n\n";
    
    for (size_t i = 0; i < rules.size(); ++i) {
        std::cout << "Rule Group " << (i + 1) << ":\n";
        std::cout << "  User-Agents: ";
        for (const auto& agent : rules[i].user_agents) {
            std::cout << "[" << agent << "] ";
        }
        std::cout << "\n";
        
        std::cout << "  Specificity: " << rules[i].specificity;
        if (rules[i].specificity == 1) std::cout << " (Wildcard)";
        else if (rules[i].specificity == 3) std::cout << " (Exact Match)";
        std::cout << "\n";
        
        if (!rules[i].disallows.empty()) {
            std::cout << "  Disallow: ";
            for (const auto& path : rules[i].disallows) {
                std::cout << "[" << path << "] ";
            }
            std::cout << "\n";
        }
        
        if (!rules[i].allows.empty()) {
            std::cout << "  Allow: ";
            for (const auto& path : rules[i].allows) {
                std::cout << "[" << path << "] ";
            }
            std::cout << "\n";
        }
        
        std::cout << "\n";
    }
    
    // Demonstrate User-Agent matching
    std::cout << "=== USER-AGENT MATCHING ===\n\n";
    
    std::vector<std::string> test_agents = {
        "googlebot",
        "googlebot/1.2",
        "MyBot/1.0",
        "bingbot",
        "slowbot",
        "unknownbot"
    };
    
    for (const auto& agent : test_agents) {
        std::cout << "Testing '" << agent << "':\n";
        
        // Count matching rule groups
        int matches = 0;
        for (const auto& rule : rules) {
            for (const auto& rule_agent : rule.user_agents) {
                if (crawler.matches_user_agent(rule_agent, agent)) {
                    matches++;
                    std::cout << "  ✓ Matches rule: " << rule_agent << "\n";
                }
            }
        }
        
        if (matches == 0) {
            std::cout << "  ✗ No matching rules (will use default - allow all)\n";
        }
        std::cout << "\n";
    }
    
    // Demonstrate normalization
    std::cout << "=== USER-AGENT NORMALIZATION ===\n\n";
    
    std::vector<std::string> agents_to_normalize = {
        "googlebot/1.2",
        "bingbot*",
        "crawler/2.0",
        "*"
    };
    
    for (const auto& agent : agents_to_normalize) {
        std::string normalized = crawler.normalize_user_agent(agent);
        std::cout << "'" << agent << "' -> '" << normalized << "'\n";
    }
    
    std::cout << "\n";
    return 0;
}
