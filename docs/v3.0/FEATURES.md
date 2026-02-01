# v3.0 - RocksDB, Gumbo Text Extraction, and Advanced Markdown Processing

## Overview

Version 3.0 introduces three major enhancements to the crawler:

1. **RocksDB Integration** - Persistent storage of URL queue and visited links
2. **Gumbo-based Text Extraction** - Advanced HTML parsing with Markdown formatting
3. **Language Detection** - Automatic programming language detection in code blocks

## Features

### 1. RocksDB Integration

#### Queue Management
- Persistent URL queue with FIFO ordering
- Automatic save/restore of crawler state
- Efficient key-value storage

```cpp
RocksDBManager db_manager("./crawler_db");
db_manager.init();
db_manager.enqueue_url("https://example.com");
std::string next_url = db_manager.dequeue_url();
```

#### Visited Links Tracking
- Track all crawled URLs
- Prevent duplicate crawling
- Generate crawl statistics

```cpp
db_manager.mark_visited("https://example.com");
if (db_manager.is_visited(url)) {
    // Skip already crawled URL
}
std::vector<std::string> visited = db_manager.get_all_visited();
```

#### HTML Caching
- Cache downloaded HTML for analysis
- Reduce re-fetching
- Fast recovery

```cpp
db_manager.cache_html(url, html_content);
std::string cached = db_manager.get_cached_html(url);
```

### 2. Gumbo-based Text Extraction

#### Markdown Formatting
HTML elements are automatically converted to Markdown:
- `<h1>`, `<h2>`, `<h3>` → `#`, `##`, `###`
- `<strong>`, `<b>` → `**text**`
- `<em>`, `<i>` → `*text*`
- `<code>` → `` `code` ``
- `<a href>` → `[text](url)`
- `<blockquote>` → `> text`
- `<li>` → `- list item`

#### Code Block Detection
```cpp
// Automatically detects and formats code blocks
TextExtraction extraction = text_extractor.extract_from_html(html, url);
// Result includes: ```js\nfunction code\n```
```

#### Configurable Element Removal
Remove unwanted elements:
```cpp
text_extractor.set_remove_selectors(
    "nav, footer, script, style, noscript, svg, "
    "[role=\"alert\"], [role=\"banner\"], [aria-modal=\"true\"]"
);
```

### 3. Language Detection

Automatic detection of programming languages in code blocks:

| Pattern | Language |
|---------|----------|
| `function name()`, `const x =`, `import {` | JavaScript |
| `<?php`, `echo` | PHP |
| `def name():`, `import module` | Python |
| `#include <lib>`, `class Name {` | C++ |
| `pub fn name()`, `fn name()` | Rust |
| `func name()`, `package main` | Go |
| `CREATE TABLE`, `SELECT * FROM` | SQL |
| `<html>`, `<!DOCTYPE` | HTML |
| `.css`, `@media` | CSS |
| `#!/bin/bash`, `$()` | Bash |

### Example: Complete Text Extraction

```cpp
#include "text_extractor.h"

TextExtractor extractor;

// Configure what to remove
extractor.set_remove_selectors(
    "nav, footer, script, style, noscript, svg, "
    "[role=\"alert\"], [role=\"banner\"]"
);

// Extract text from HTML
TextExtraction result = extractor.extract_from_html(html_content, url);

// Result contains:
// - title: Page title
// - text: Markdown formatted text with # for headings, ** for bold, etc.
// - code_blocks: Array of code with language detected
// - links: Array of extracted URLs
// - plain_text: Plain text without markdown
```

## RocksDB Structure

Data is organized with prefixes:

```
queue:00000000  <- URL queue (FIFO)
queue:00000001
queue:...

visited:https://example.com  <- Visited URLs
visited:https://another.com

cache:https://example.com  <- HTML cache
cache:https://another.com
```

## Text Extraction Pipeline

1. **Parse HTML** with Gumbo
2. **Remove unwanted elements** (nav, footer, scripts, etc.)
3. **Convert headings** to Markdown (#, ##, ###)
4. **Format text** (bold, italic, code)
5. **Detect code blocks** and language
6. **Extract links**
7. **Normalize** whitespace
8. **Return** Markdown formatted text

## Code Block Detection

The system looks for:
- `<pre>` and `<code>` tags
- `<code>` elements with syntax highlighting classes
- Indented code blocks

Language detection uses regex patterns to identify:
- JavaScript: `function`, `const`, `import`
- Python: `def`, `import`, `class`
- SQL: `CREATE`, `SELECT`, `INSERT`
- And 7+ other languages

## Performance Considerations

### RocksDB
- Compression enabled (Snappy)
- O(log n) key lookup
- Efficient batch operations
- Automatic compaction

### Text Extraction
- Single-pass parsing with Gumbo
- Regex-based language detection
- Minimal memory overhead
- ~1-10ms per document

## API Reference

### RocksDBManager

```cpp
class RocksDBManager {
    // Initialize
    bool init();
    
    // Queue
    bool enqueue_url(const std::string& url);
    std::string dequeue_url();
    bool has_queued_urls();
    int get_queue_size();
    
    // Visited
    bool mark_visited(const std::string& url);
    bool is_visited(const std::string& url);
    std::vector<std::string> get_all_visited();
    int get_visited_count();
    
    // Cache
    bool cache_html(const std::string& url, const std::string& html);
    std::string get_cached_html(const std::string& url);
    bool has_cached_html(const std::string& url);
    
    // Management
    std::string get_stats();
    void clear_all();
};
```

### TextExtractor

```cpp
class TextExtractor {
    TextExtraction extract_from_html(
        const std::string& html,
        const std::string& url
    );
    
    void set_remove_selectors(const std::string& selectors);
};

struct TextExtraction {
    std::string title;
    std::string text;                    // Markdown
    std::string plain_text;              // Plain text
    std::vector<std::string> code_blocks;
    std::vector<std::string> links;
};
```

## Building

```bash
# Install dependencies
sudo apt-get install librocksdb-dev libgumbo-dev

# Build
cd /workspaces/dataset/build
cmake ..
make
```

## Usage Example

```cpp
#include "crawler.h"
#include "rocksdb_manager.h"
#include "text_extractor.h"

int main() {
    // Initialize database
    RocksDBManager db("./crawler_db");
    db.init();
    
    // Initialize text extractor
    TextExtractor extractor;
    
    // Queue URL
    db.enqueue_url("https://example.com");
    
    // Process queue
    while (db.has_queued_urls()) {
        std::string url = db.dequeue_url();
        
        if (db.is_visited(url)) {
            continue;
        }
        
        // Fetch and process
        std::string html = fetch_html(url);
        TextExtraction text = extractor.extract_from_html(html, url);
        
        // Cache and mark visited
        db.cache_html(url, html);
        db.mark_visited(url);
        
        // Use extracted text
        std::cout << "Title: " << text.title << "\n";
        std::cout << "Content:\n" << text.text << "\n";
    }
    
    // Print statistics
    std::cout << db.get_stats();
    
    return 0;
}
```

## Technical Stack

- **RocksDB**: Key-value database (8.9.1)
- **Gumbo**: HTML parser (0.12.0)
- **C++17**: Modern C++ standard
- **CMake 3.20+**: Build system

## Version Information

- **Version**: 3.0
- **Date**: January 2026
- **Status**: Production Ready
- **Tests**: All previous tests passing
- **Backward Compatible**: Yes (optional features)

## Next Steps

Potential enhancements:
1. Parallel text extraction
2. ML-based language detection
3. Custom CSS selector engine
4. Incremental indexing
5. Compression options

---

**Ready to crawl and extract!** 🚀
