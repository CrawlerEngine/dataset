#include "rocksdb_manager.h"
#include <iostream>
#include <cassert>
#include <filesystem>

void test_persistence(const std::string& db_path) {
    // Open database to check persistence
    RocksDBManager db2(db_path);
    assert(db2.init());
    assert(db2.get_visited_count() == 2);
    assert(db2.is_visited("https://example.com"));
    assert(db2.has_cached_html("https://example.com"));
    std::cout << "✓ Data persisted across database reopens" << std::endl;
}

int main() {
    std::cout << "=== Testing RocksDBManager ===" << std::endl;
    
    // Create temp directory
    std::string db_path = "/tmp/test_crawler_db";
    std::filesystem::remove_all(db_path);  // Clean up old test
    
    {
        RocksDBManager db(db_path);
        assert(db.init());
        std::cout << "✓ Database initialized" << std::endl;
        
        // Test 1: Queue operations
        std::cout << "\n--- Queue Operations ---" << std::endl;
        assert(db.enqueue_url("https://example.com"));
        assert(db.enqueue_url("https://test.com"));
        assert(db.enqueue_url("https://another.com"));
        std::cout << "✓ 3 URLs queued" << std::endl;
        
        assert(db.get_queue_size() == 3);
        assert(db.has_queued_urls());
        std::cout << "✓ Queue size: " << db.get_queue_size() << std::endl;
        
        std::string first = db.dequeue_url();
        assert(first == "https://example.com");
        std::cout << "✓ Dequeued: " << first << std::endl;
        
        assert(db.get_queue_size() == 2);
        std::cout << "✓ Queue size after dequeue: " << db.get_queue_size() << std::endl;
        
        // Test 2: Visited links
        std::cout << "\n--- Visited Links ---" << std::endl;
        assert(db.mark_visited("https://example.com"));
        assert(db.mark_visited("https://visited.com"));
        std::cout << "✓ 2 URLs marked as visited" << std::endl;
        
        assert(db.is_visited("https://example.com"));
        assert(!db.is_visited("https://not-visited.com"));
        std::cout << "✓ Visit status checked" << std::endl;
        
        assert(db.get_visited_count() == 2);
        std::cout << "✓ Visited count: " << db.get_visited_count() << std::endl;
        
        // Test 3: HTML caching
        std::cout << "\n--- HTML Caching ---" << std::endl;
        std::string test_html = "<html><body>Test content</body></html>";
        assert(db.cache_html("https://example.com", test_html));
        std::cout << "✓ HTML cached" << std::endl;
        
        assert(db.has_cached_html("https://example.com"));
        assert(!db.has_cached_html("https://not-cached.com"));
        std::cout << "✓ Cache lookup works" << std::endl;
        
        std::string cached = db.get_cached_html("https://example.com");
        assert(cached == test_html);
        std::cout << "✓ Cached content retrieved" << std::endl;
        
        // Test 4: Statistics
        std::cout << "\n--- Statistics ---" << std::endl;
        std::string stats = db.get_stats();
        std::cout << "✓ Statistics:\n" << stats << std::endl;
    }
    
    // Test 5: Persistence (create new instance and check data)
    std::cout << "\n--- Persistence ---" << std::endl;
    test_persistence(db_path);
    
    // Clean up
    std::filesystem::remove_all(db_path);
    std::cout << "\n✓ All RocksDBManager tests passed!" << std::endl;
    
    return 0;
}
