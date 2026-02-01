#include "crawler.h"
#include <gtest/gtest.h>

class RobotsWildcardTest : public ::testing::Test {
protected:
    WebCrawler crawler{"TestBot/1.0"};
};

TEST_F(RobotsWildcardTest, WildcardStar) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/*.pdf"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/document.pdf", "TestBot"));
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/files/report.pdf", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/page.html", "TestBot"));
}

TEST_F(RobotsWildcardTest, WildcardEndOfUrl) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/*.php$"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/index.php", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/index.php.bak", "TestBot"));
}

TEST_F(RobotsWildcardTest, AdminDirectoryBlock) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/admin/*"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/admin/page.html", "TestBot"));
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/admin/", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/page.html", "TestBot"));
}

TEST_F(RobotsWildcardTest, LongestMatchWins) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/fish", "/fish*.php"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    // /fish*.php should match fish.php (longer pattern)
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/fish.php", "TestBot"));
}

TEST_F(RobotsWildcardTest, AllowBeatDisallow) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/admin/*"};
    rule.allows = {"/admin/public/*"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/admin/private/", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/admin/public/page.html", "TestBot"));
}

TEST_F(RobotsWildcardTest, CaseSensitive) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/Admin/"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/Admin/", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/admin/", "TestBot"));
}

TEST_F(RobotsWildcardTest, SpecialCharactersInPath) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/*.cgi$"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/script.cgi", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/script.cgi.txt", "TestBot"));
}
