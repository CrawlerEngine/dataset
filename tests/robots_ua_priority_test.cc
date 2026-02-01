#include "crawler.h"
#include <gtest/gtest.h>

class RobotsUAPriorityTest : public ::testing::Test {
protected:
    WebCrawler crawler{"TestBot/1.0"};
};

TEST_F(RobotsUAPriorityTest, ExactUserAgentMatch) {
    std::vector<RobotRule> rules;
    RobotRule rule1;
    rule1.user_agents = {"Googlebot"};
    rule1.disallows = {"/admin/"};
    rule1.specificity = 2;
    rules.push_back(rule1);
    
    RobotRule rule2;
    rule2.user_agents = {"*"};
    rule2.disallows = {"/private/"};
    rule2.specificity = 1;
    rules.push_back(rule2);
    
    // Test with Googlebot
    bool allowed = crawler.is_path_allowed(rules, "/admin/", "Googlebot");
    EXPECT_FALSE(allowed);
}

TEST_F(RobotsUAPriorityTest, WildcardFallback) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/blocked/"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    bool allowed = crawler.is_path_allowed(rules, "/blocked/", "AnyBot");
    EXPECT_FALSE(allowed);
}

TEST_F(RobotsUAPriorityTest, AllowOverride) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/admin/"};
    rule.allows = {"/admin/public/"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    bool allowed = crawler.is_path_allowed(rules, "/admin/public/", "TestBot");
    EXPECT_TRUE(allowed);
}
