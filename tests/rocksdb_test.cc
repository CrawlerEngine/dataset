#include "rocksdb_manager.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <random>
#include <chrono>

class RocksDBManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create unique test directory using process ID and timestamp
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        db_path = "/tmp/test_crawler_db_" + std::to_string(getpid()) + "_" + std::to_string(now);
        std::filesystem::create_directories(db_path);
        db = std::make_unique<RocksDBManager>(db_path);
        ASSERT_TRUE(db->init());
    }

    void TearDown() override {
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    std::unique_ptr<RocksDBManager> db;
    std::string db_path;
};

TEST_F(RocksDBManagerTest, DatabaseInitialization) {
    // Database is already initialized in SetUp()
    // This test verifies that initialization succeeded
    EXPECT_TRUE(db->get_queue_size() >= 0);
}

TEST_F(RocksDBManagerTest, EnqueueAndDequeue) {
    ASSERT_TRUE(db->enqueue_url("https://example.com"));
    ASSERT_TRUE(db->enqueue_url("https://test.com"));
    ASSERT_TRUE(db->enqueue_url("https://another.com"));
    
    EXPECT_EQ(db->dequeue_url(), "https://example.com");
    EXPECT_EQ(db->dequeue_url(), "https://test.com");
    EXPECT_EQ(db->dequeue_url(), "https://another.com");
}

TEST_F(RocksDBManagerTest, QueueSize) {
    ASSERT_TRUE(db->enqueue_url("https://example.com"));
    ASSERT_TRUE(db->enqueue_url("https://test.com"));
    
    EXPECT_EQ(db->get_queue_size(), 2);
    EXPECT_TRUE(db->has_queued_urls());
    
    db->dequeue_url();
    EXPECT_EQ(db->get_queue_size(), 1);
    
    db->dequeue_url();
    EXPECT_EQ(db->get_queue_size(), 0);
    EXPECT_FALSE(db->has_queued_urls());
}

TEST_F(RocksDBManagerTest, VisitedTracking) {
    ASSERT_TRUE(db->mark_visited("https://example.com"));
    ASSERT_TRUE(db->mark_visited("https://visited.com"));
    
    EXPECT_TRUE(db->is_visited("https://example.com"));
    EXPECT_TRUE(db->is_visited("https://visited.com"));
    EXPECT_FALSE(db->is_visited("https://not-visited.com"));
    
    EXPECT_EQ(db->get_visited_count(), 2);
}

TEST_F(RocksDBManagerTest, HTMLCaching) {
    std::string test_html = "<html><body>Test content</body></html>";
    
    ASSERT_TRUE(db->cache_html("https://example.com", test_html));
    EXPECT_TRUE(db->has_cached_html("https://example.com"));
    EXPECT_FALSE(db->has_cached_html("https://not-cached.com"));
    
    std::string cached = db->get_cached_html("https://example.com");
    EXPECT_EQ(cached, test_html);
}

TEST_F(RocksDBManagerTest, Persistence) {
    // Add data
    ASSERT_TRUE(db->enqueue_url("https://example.com"));
    ASSERT_TRUE(db->mark_visited("https://visited.com"));
    ASSERT_TRUE(db->cache_html("https://example.com", "<html>Test</html>"));
    
    // Reset database instance
    db.reset();
    
    // Create new instance and verify data persists
    db = std::make_unique<RocksDBManager>(db_path);
    ASSERT_TRUE(db->init());
    
    EXPECT_EQ(db->get_visited_count(), 1);
    EXPECT_TRUE(db->is_visited("https://visited.com"));
    EXPECT_TRUE(db->has_cached_html("https://example.com"));
}

TEST_F(RocksDBManagerTest, Statistics) {
    ASSERT_TRUE(db->enqueue_url("https://example.com"));
    ASSERT_TRUE(db->mark_visited("https://visited.com"));
    
    std::string stats = db->get_stats();
    EXPECT_FALSE(stats.empty());
    EXPECT_TRUE(stats.find("Queued URLs") != std::string::npos);
    EXPECT_TRUE(stats.find("Visited URLs") != std::string::npos);
}
