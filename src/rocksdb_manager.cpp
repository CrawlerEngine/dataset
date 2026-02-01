#include "rocksdb_manager.h"
#include "logger.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <sstream>
#include <iomanip>

RocksDBManager::RocksDBManager(const std::string& db_path)
    : db_path_(db_path), db_(nullptr) {
    options_ = std::make_unique<rocksdb::Options>();
}

RocksDBManager::~RocksDBManager() {
    if (db_) {
        delete db_;
    }
}

bool RocksDBManager::init() {
    if (!options_) {
        Logger::instance().error("RocksDB: Options not initialized");
        return false;
    }
    
    options_->create_if_missing = true;
    options_->compression = rocksdb::kSnappyCompression;
    
    rocksdb::Status status = rocksdb::DB::Open(*options_, db_path_, &db_);
    if (!status.ok()) {
        Logger::instance().error("RocksDB: Failed to open database: " + status.ToString());
        return false;
    }
    
    Logger::instance().info("RocksDB: Database opened successfully at " + db_path_);
    return true;
}

bool RocksDBManager::enqueue_url(const std::string& url) {
    if (!db_) return false;
    
    // Get current tail index
    std::string tail_str;
    int tail = 0;
    if (db_->Get(rocksdb::ReadOptions(), "queue:tail", &tail_str).ok()) {
        tail = std::stoi(tail_str);
    }
    
    // Store URL at tail
    std::string key = make_queue_key(tail);
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, url);
    if (!status.ok()) {
        Logger::instance().error("RocksDB: Failed to enqueue URL: " + status.ToString());
        return false;
    }
    
    // Increment tail
    status = db_->Put(rocksdb::WriteOptions(), "queue:tail", std::to_string(tail + 1));
    if (!status.ok()) {
        Logger::instance().error("RocksDB: Failed to update queue tail: " + status.ToString());
        return false;
    }
    
    return true;
}

std::string RocksDBManager::dequeue_url() {
    if (!db_) return "";
    
    // Get current head index
    std::string head_str;
    int head = 0;
    if (db_->Get(rocksdb::ReadOptions(), "queue:head", &head_str).ok()) {
        head = std::stoi(head_str);
    }
    
    // Get tail index
    std::string tail_str;
    int tail = 0;
    if (db_->Get(rocksdb::ReadOptions(), "queue:tail", &tail_str).ok()) {
        tail = std::stoi(tail_str);
    }
    
    // Check if queue is empty
    if (head >= tail) {
        return "";
    }
    
    // Get URL at head
    std::string key = make_queue_key(head);
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
    if (!status.ok()) {
        return "";
    }
    
    // Increment head
    status = db_->Put(rocksdb::WriteOptions(), "queue:head", std::to_string(head + 1));
    if (!status.ok()) {
        Logger::instance().error("RocksDB: Failed to update queue head: " + status.ToString());
        return "";
    }
    
    return value;
}

bool RocksDBManager::has_queued_urls() {
    return get_queue_size() > 0;
}

int RocksDBManager::get_queue_size() {
    if (!db_) return 0;
    
    // Get head and tail indices
    std::string head_str, tail_str;
    int head = 0, tail = 0;
    
    if (db_->Get(rocksdb::ReadOptions(), "queue:head", &head_str).ok()) {
        head = std::stoi(head_str);
    }
    
    if (db_->Get(rocksdb::ReadOptions(), "queue:tail", &tail_str).ok()) {
        tail = std::stoi(tail_str);
    }
    
    return (tail >= head) ? (tail - head) : 0;
}

bool RocksDBManager::mark_visited(const std::string& url) {
    if (!db_) return false;
    
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), 
                                       make_visited_key(url), "1");
    return status.ok();
}

bool RocksDBManager::is_visited(const std::string& url) {
    if (!db_) return false;
    
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), 
                                       make_visited_key(url), &value);
    return status.ok();
}

std::vector<std::string> RocksDBManager::get_all_visited() {
    std::vector<std::string> visited;
    if (!db_) return visited;
    
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    std::string prefix = "visited:";
    
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key.substr(0, prefix.length()) != prefix) {
            break;
        }
        // Extract URL from key (remove "visited:" prefix)
        visited.push_back(key.substr(prefix.length()));
    }
    
    delete it;
    return visited;
}

int RocksDBManager::get_visited_count() {
    return get_all_visited().size();
}

bool RocksDBManager::cache_html(const std::string& url, const std::string& html) {
    if (!db_) return false;
    
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), 
                                       make_cache_key(url), html);
    return status.ok();
}

std::string RocksDBManager::get_cached_html(const std::string& url) {
    if (!db_) return "";
    
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), 
                                       make_cache_key(url), &value);
    
    return status.ok() ? value : "";
}

bool RocksDBManager::has_cached_html(const std::string& url) {
    if (!db_) return false;
    
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), 
                                       make_cache_key(url), &value);
    return status.ok();
}

std::string RocksDBManager::get_stats() {
    std::ostringstream oss;
    oss << "RocksDB Statistics:\n";
    oss << "  Queued URLs: " << get_queue_size() << "\n";
    oss << "  Visited URLs: " << get_visited_count() << "\n";
    
    return oss.str();
}

void RocksDBManager::clear_all() {
    if (!db_) return;
    
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        db_->Delete(rocksdb::WriteOptions(), it->key());
    }
    delete it;
}

std::string RocksDBManager::make_queue_key(int index) const {
    std::ostringstream oss;
    oss << "queue:" << std::setfill('0') << std::setw(8) << index;
    return oss.str();
}

std::string RocksDBManager::make_visited_key(const std::string& url) const {
    return "visited:" + url;
}

std::string RocksDBManager::make_cache_key(const std::string& url) const {
    return "cache:" + url;
}
