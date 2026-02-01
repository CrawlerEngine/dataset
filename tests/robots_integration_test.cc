#include "crawler.h"
#include <gtest/gtest.h>

class RobotsIntegrationTest : public ::testing::Test {
protected:
    WebCrawler crawler{"TestBot/1.0"};
};

TEST_F(RobotsIntegrationTest, BasicRobotsParsing) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/admin/", "/private/"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/admin/", "TestBot"));
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/private/", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/public/", "TestBot"));
}

TEST_F(RobotsIntegrationTest, MultipleRules) {
    std::vector<RobotRule> rules;
    
    RobotRule rule1;
    rule1.user_agents = {"Googlebot"};
    rule1.disallows = {"/google-blocked/"};
    rule1.specificity = 2;
    rules.push_back(rule1);
    
    RobotRule rule2;
    rule2.user_agents = {"*"};
    rule2.disallows = {"/general-blocked/"};
    rule2.specificity = 1;
    rules.push_back(rule2);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/google-blocked/", "Googlebot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/google-blocked/", "OtherBot"));
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/general-blocked/", "AnyBot"));
}

TEST_F(RobotsIntegrationTest, AllowAndDisallow) {
    std::vector<RobotRule> rules;
    RobotRule rule;
    rule.user_agents = {"*"};
    rule.disallows = {"/admin/"};
    rule.allows = {"/admin/public/"};
    rule.specificity = 1;
    rules.push_back(rule);
    
    EXPECT_FALSE(crawler.is_path_allowed(rules, "/admin/", "TestBot"));
    EXPECT_TRUE(crawler.is_path_allowed(rules, "/admin/public/", "TestBot"));
}
