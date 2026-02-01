# DEVELOPER.md - Developer Guide

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│         Dataset Crawler Architecture               │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────────┐         ┌──────────────┐        │
│  │   WebCrawler │         │Advanced      │        │
│  │              │────────>│  Crawler     │        │
│  │  - fetch()   │         │              │        │
│  │  - crawl()   │         │ Thread Pool  │        │
│  └──────────────┘         └──────────────┘        │
│         │                        │                 │
│         │    ┌──────────────┐    │                │
│         └───>│DataRecord    │<───┘                │
│              │              │                      │
│              │ - url        │                      │
│              │ - title      │                      │
│              │ - content    │                      │
│              │ - timestamp  │                      │
│              │ - status_code│                      │
│              └──────────────┘                      │
│                     │                              │
│                     │                              │
│         ┌───────────▼──────────────┐              │
│         │ ParquetDatasetWriter     │              │
│         │                          │              │
│         │ - write_records()        │              │
│         │ - append_records()       │              │
│         │ - write_csv()            │              │
│         └───────────┬──────────────┘              │
│                     │                              │
│         ┌───────────▼──────────────┐              │
│         │  Output Files            │              │
│         │                          │              │
│         │ - dataset.parquet        │              │
│         │ - dataset.csv            │              │
│         └──────────────────────────┘              │
│                                                     │
└─────────────────────────────────────────────────────┘
```

## Component Details

### 1. WebCrawler Class

**File**: `include/crawler.h`, `src/crawler.cpp`

**Responsibility**: Single-threaded HTTP fetching

**Key Methods**:
- `fetch(url)` - Fetch single page
- `crawl_urls(urls)` - Fetch multiple pages sequentially
- `set_timeout(seconds)` - Configure request timeout
- `add_header(key, value)` - Add custom HTTP headers

**Implementation Details**:
- Uses libcurl for HTTP requests
- Extracts page title using regex
- Handles HTTP errors gracefully
- Adds user-agent and default headers

### 2. AdvancedCrawler Class

**File**: `include/advanced_crawler.h`, `src/advanced_crawler.cpp`

**Responsibility**: Multi-threaded crawling with thread pool

**Key Methods**:
- `crawl_parallel(urls)` - Fetch with multiple threads
- `crawl_from_file(filename)` - Read URLs from file and crawl
- `get_stats()` - Return crawling statistics
- `set_thread_count(n)` - Configure thread pool size

**Design Pattern**: Thread Pool with Work Queue

**Synchronization**:
- `queue_mutex_` - Protects URL queue
- `results_mutex_` - Protects results vector
- `cv_` - Condition variable for thread coordination

### 3. ParquetDatasetWriter Class

**File**: `include/dataset_writer.h`, `src/dataset_writer.cpp`

**Responsibility**: Writing data to Parquet and CSV formats

**Key Methods**:
- `write_records(path, records)` - Write to Parquet
- `append_records(path, records)` - Append to existing Parquet
- `write_csv(path, records)` - Export to CSV

**Implementation**:
- Uses Apache Arrow for Parquet operations
- Automatic schema inference from data
- Snappy compression by default

### 4. DataRecord Structure

```cpp
struct DataRecord {
    std::string url;        // Source URL
    std::string title;      // Page title from <title> tag
    std::string content;    // Full HTML content
    std::string timestamp;  // Collection time
    int status_code;        // HTTP status (200, 404, etc.)
};
```

**Parquet Schema**:
```
url         : utf8
title       : utf8
content     : utf8
timestamp   : utf8
status_code : int32
```

## Building & Compilation

### CMake Build System

**CMakeLists.txt** organizes:
- C++17 standard requirement
- External dependencies (CURL, Arrow, Parquet)
- Compiler flags and warnings
- Executable target

### Compilation Flags

```cmake
# GCC/Clang
-Wall -Wextra -Wpedantic

# MSVC
/W4
```

## Dependencies

### System Libraries

1. **libcurl** (HTTP client)
   - Package: `libcurl4-openssl-dev`
   - Function: HTTP requests, redirects, SSL/TLS

2. **Apache Arrow** (columnar data format)
   - Packages: `libparquet-dev`, `libarrow-dev`
   - Function: Parquet reading/writing, data serialization

3. **CMake** (build system)
   - Package: `cmake`
   - Version: >= 3.20

### Build Tools

- GCC/Clang C++ compiler (C++17 support)
- Make or Ninja build system

## Code Examples

### Example 1: Basic Crawling

```cpp
#include "crawler.h"
#include "dataset_writer.h"

int main() {
    WebCrawler crawler;
    crawler.set_timeout(30);
    
    std::vector<std::string> urls = {"https://example.com"};
    auto records = crawler.crawl_urls(urls);
    
    ParquetDatasetWriter writer;
    writer.write_records("output.parquet", records);
    
    return 0;
}
```

### Example 2: Parallel Crawling

```cpp
#include "advanced_crawler.h"

int main() {
    AdvancedCrawler crawler(4);  // 4 threads
    
    std::vector<std::string> urls = {
        "https://example.com/1",
        "https://example.com/2",
        "https://example.com/3"
    };
    
    auto records = crawler.crawl_parallel(urls);
    
    auto stats = crawler.get_stats();
    std::cout << "Success: " << stats.successful << std::endl;
    std::cout << "Failed: " << stats.failed << std::endl;
    
    ParquetDatasetWriter writer;
    writer.write_records("output.parquet", records);
    
    return 0;
}
```

## Performance Characteristics

### Crawling Speed

- **Single-threaded**: 1-5 requests/second (network dependent)
- **Multi-threaded**: 5-20+ requests/second (8 threads)
- **Bottleneck**: Network latency, not CPU

### File Size

| Format | Typical Size | Compression |
|--------|-------------|-------------|
| Raw HTML | 500 MB | N/A |
| CSV | 350 MB | Text |
| Parquet | 50-100 MB | Snappy |

### Memory Usage

- Per-thread: ~10-50 MB
- Queue buffer: ~1-10 MB
- Total for 4 threads: ~50-200 MB

## Testing

### Manual Testing

```bash
# Test single URL
./build/crawler

# Test from file
./build/crawler < urls.txt

# Test with Python
python3 scripts/parquet_utils.py info dataset.parquet
```

### Python Data Validation

```python
import pandas as pd

df = pd.read_parquet('dataset.parquet')

# Check data integrity
assert df['url'].notna().all()
assert df['status_code'].dtype == 'int32'
assert all(df['status_code'] > 0)
```

## Extending the Crawler

### Adding Custom Parsing

```cpp
class AdvancedDataRecord : public DataRecord {
public:
    std::string extract_metadata() {
        // Custom parsing logic
        return parsed_data;
    }
};
```

### Adding Proxy Support

```cpp
crawler_->add_header("X-Proxy-Authorization", "Bearer token");
// Or modify CURL options in crawler.cpp
```

### Adding Database Output

```cpp
class DatabaseWriter {
    void write_to_db(const std::vector<DataRecord>& records) {
        // Database implementation
    }
};
```

## Common Issues & Solutions

### Issue: Slow Crawling
**Solution**: Increase thread count in AdvancedCrawler

```cpp
AdvancedCrawler crawler(16);  // More threads
```

### Issue: Memory Usage
**Solution**: Use batch processing

```cpp
for (auto& batch : batches) {
    auto records = crawler.crawl_parallel(batch);
    writer.write_records("part.parquet", records);
}
```

### Issue: SSL/TLS Errors
**Solution**: Disable verification (testing only!)

```cpp
// In crawler.cpp, modify CURL options:
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
```

## Performance Optimization Tips

1. **Use thread pool** instead of single-threaded
2. **Batch Parquet writes** for large datasets
3. **Enable compression** (Snappy is default)
4. **Reuse HTTP connections** (libcurl does this)
5. **Use reasonable timeouts** (30-60 seconds)
6. **Monitor memory** during large crawls

## Contributing

### Code Style
- Use CamelCase for classes
- Use snake_case for variables
- Add comments for complex logic
- Keep functions focused and small

### Adding Features
1. Create header in `include/`
2. Implement in `src/`
3. Add examples in `examples/`
4. Update README and QUICKSTART

## References

- [libcurl Documentation](https://curl.se/libcurl/c/)
- [Apache Arrow C++ API](https://arrow.apache.org/docs/cpp/)
- [Parquet Format](https://parquet.apache.org/)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)
