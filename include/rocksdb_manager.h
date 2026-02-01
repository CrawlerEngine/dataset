#ifndef ROCKSDB_MANAGER_H
#define ROCKSDB_MANAGER_H

#include <string>
#include <vector>
#include <memory>

namespace rocksdb {
    class DB;
    class Options;
}

class RocksDBManager {
public:
    RocksDBManager(const std::string& db_path);
    ~RocksDBManager();

    // Initialize database
    bool init();
    
    // Queue operations
    bool enqueue_url(const std::string& url);
    std::string dequeue_url();
    bool has_queued_urls();
    int get_queue_size();
    
    // Visited links operations
    bool mark_visited(const std::string& url);
    bool is_visited(const std::string& url);
    std::vector<std::string> get_all_visited();
    int get_visited_count();
    
    // Cache operations
    bool cache_html(const std::string& url, const std::string& html);
    std::string get_cached_html(const std::string& url);
    bool has_cached_html(const std::string& url);
    
    // Statistics
    std::string get_stats();
    
    // Utility
    void clear_all();
    
private:
    std::string db_path_;
    rocksdb::DB* db_;
    std::unique_ptr<rocksdb::Options> options_;
    
    std::string make_queue_key(int index) const;
    std::string make_visited_key(const std::string& url) const;
    std::string make_cache_key(const std::string& url) const;
};

#endif // ROCKSDB_MANAGER_H
