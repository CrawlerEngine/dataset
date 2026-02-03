#include "rocksdb_manager.h"
#include "logger.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <sstream>
#include <iomanip>
#include <memory>

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

bool RocksDBManager::enqueue_url(const std::string& url, int priority) {
    if (!db_) return false;
    
    // Get current tail index for priority
    std::string tail_str;
    int tail = 0;
    std::string tail_key = make_priority_tail_key(priority);
    if (db_->Get(rocksdb::ReadOptions(), tail_key, &tail_str).ok()) {
        tail = std::stoi(tail_str);
    }
    
    // Store URL at tail for priority queue
    std::string key = make_priority_queue_key(priority, tail);
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, url);
    if (!status.ok()) {
        Logger::instance().error("RocksDB: Failed to enqueue URL: " + status.ToString());
        return false;
    }
    
    // Increment tail
    status = db_->Put(rocksdb::WriteOptions(), tail_key, std::to_string(tail + 1));
    if (!status.ok()) {
        Logger::instance().error("RocksDB: Failed to update queue tail: " + status.ToString());
        return false;
    }

    // Increment queue size
    std::string size_str;
    int size = 0;
    if (db_->Get(rocksdb::ReadOptions(), "pqueue:size", &size_str).ok()) {
        size = std::stoi(size_str);
    }
    db_->Put(rocksdb::WriteOptions(), "pqueue:size", std::to_string(size + 1));
    
    return true;
}

std::string RocksDBManager::dequeue_url() {
    if (!db_) return "";

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    const std::string prefix = "pqueue:item:";
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key.compare(0, prefix.size(), prefix) != 0) {
            break;
        }
        std::string value = it->value().ToString();
        rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
        if (!status.ok()) {
            Logger::instance().error("RocksDB: Failed to dequeue URL: " + status.ToString());
            return "";
        }

        std::string size_str;
        int size = 0;
        if (db_->Get(rocksdb::ReadOptions(), "pqueue:size", &size_str).ok()) {
            size = std::stoi(size_str);
        }
        if (size > 0) {
            db_->Put(rocksdb::WriteOptions(), "pqueue:size", std::to_string(size - 1));
        }

        return value;
    }

    return "";
}

bool RocksDBManager::has_queued_urls() {
    return get_queue_size() > 0;
}

int RocksDBManager::get_queue_size() {
    if (!db_) return 0;

    std::string size_str;
    if (db_->Get(rocksdb::ReadOptions(), "pqueue:size", &size_str).ok()) {
        return std::stoi(size_str);
    }

    int count = 0;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    const std::string prefix = "pqueue:item:";
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key.compare(0, prefix.size(), prefix) != 0) {
            break;
        }
        ++count;
    }
    return count;
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

bool RocksDBManager::add_link_edge(const std::string& from_url, const std::string& to_url) {
    if (!db_) return false;

    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(),
                                      make_link_edge_key(from_url, to_url), "1");
    return status.ok();
}

std::vector<std::string> RocksDBManager::get_outgoing_links(const std::string& from_url) {
    std::vector<std::string> links;
    if (!db_) return links;

    std::string prefix = make_link_prefix(from_url);
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        if (key.compare(0, prefix.size(), prefix) != 0) {
            break;
        }
        links.push_back(key.substr(prefix.size()));
    }

    return links;
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

std::string RocksDBManager::make_visited_key(const std::string& url) const {
    return "visited:" + url;
}

std::string RocksDBManager::make_cache_key(const std::string& url) const {
    return "cache:" + url;
}

std::string RocksDBManager::make_priority_queue_key(int priority, int index) const {
    std::ostringstream oss;
    oss << "pqueue:item:" << std::setfill('0') << std::setw(4) << priority
        << ":" << std::setw(12) << index;
    return oss.str();
}

std::string RocksDBManager::make_priority_tail_key(int priority) const {
    std::ostringstream oss;
    oss << "pqueue:tail:" << std::setfill('0') << std::setw(4) << priority;
    return oss.str();
}

std::string RocksDBManager::make_link_edge_key(const std::string& from_url, const std::string& to_url) const {
    return "graph:" + from_url + "->" + to_url;
}

std::string RocksDBManager::make_link_prefix(const std::string& from_url) const {
    return "graph:" + from_url + "->";
}
