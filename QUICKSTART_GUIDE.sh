#!/usr/bin/env bash

# 🚀 Quick Setup Guide for Dataset Crawler

cat << 'EOF'
╔════════════════════════════════════════════════════════════════╗
║    🚀 Dataset Crawler - Quick Setup Guide                     ║
║    C++ Web Crawler for Dataset Collection                     ║
╚════════════════════════════════════════════════════════════════╝

## ⚡ FASTEST START (30 seconds)

1. Build the project:
   cd build && cmake .. && make

2. Crawl a single URL:
   ./crawler --url "https://example.com"

3. Output files created in ./output/:
   - dataset.json
   - dataset.csv

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 🎯 COMMON USE CASES

### 1️⃣ Crawl your own website
   ./crawler --url "https://yoursite.com"

### 2️⃣ Crawl multiple URLs
   ./crawler --urls "https://site1.com,https://site2.com,https://site3.com"

### 3️⃣ Use config file
   ./crawler --config ../config.json

### 4️⃣ Custom parameters
   ./crawler --url "https://site.com" --timeout 60 --user-agent "MyBot/1.0"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ✅ ALLOWED SITES (OK TO CRAWL)

✅ Wikipedia       - https://wikipedia.org
✅ GitHub          - https://github.com
✅ ArXiv           - https://arxiv.org (research papers)
✅ Stack Overflow  - https://stackoverflow.com
✅ Medium          - https://medium.com
✅ Your own site   - https://yoursite.com

Example:
   ./crawler --url "https://en.wikipedia.org/wiki/Machine_learning"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ❌ PROHIBITED SITES (DO NOT CRAWL)

❌ LinkedIn       - STRICTLY FORBIDDEN ⛔
   Why? Terms of Service prohibit automated crawling
   Risk: Account ban, lawsuits, $5K-$100K+ fines

Use official LinkedIn API instead:
   https://developers.linkedin.com/

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 📋 ALL COMMAND-LINE OPTIONS

--url URL              Single URL to crawl
--urls "url1,url2"     Multiple URLs (comma-separated)
--timeout SECONDS      Request timeout (default: 30)
--user-agent STRING    Custom User-Agent identifier
--config FILE          Load settings from config.json
--output-dir PATH      Output directory (default: ./output)

Examples:
   ./crawler --url "https://example.com" --timeout 60
   ./crawler --urls "url1,url2,url3" --user-agent "MyBot/1.0"
   ./crawler --config config.json --output-dir ./data

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 📊 OUTPUT FILES

JSON Format (dataset.json):
{
  "url": "https://example.com",
  "title": "Page Title",
  "content_length": 12345,
  "timestamp": "2026-01-25 08:19:10",
  "status_code": 200
}

CSV Format (dataset.csv):
url,title,content_length,timestamp,status_code
https://example.com,"Page Title",12345,"2026-01-25 08:19:10",200

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 🔍 LOGGING FORMAT

2026-01-25T08:19:09.961Z INFO   Crawling started
2026-01-25T08:19:10.015Z INFO   https://example.com [200]
2026-01-25T08:19:10.154Z INFO   Crawling complete

Colors:
🟢 INFO  (green)   - Success/normal operation
🟡 WARN  (yellow)  - Warnings/potential issues
🔴 ERROR (red)     - Errors
🔵 DEBUG (cyan)    - Diagnostic info

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ⚙️ CONFIG FILE (config.json)

{
  "crawler": {
    "timeout": 30,
    "user_agent": "MyBot/1.0",
    "respect_robots_txt": true,
    "respect_meta_tags": true
  },
  "output": {
    "format": "both",  // "json", "csv", or "both"
    "output_dir": "./output"
  },
  "urls": [
    "https://example.com",
    "https://example.org"
  ],
  "headers": {
    "Accept-Language": "en-US,en;q=0.9"
  }
}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 🛡️ ETHICAL CRAWLING (AUTOMATIC)

The crawler automatically:
✅ Respects robots.txt files
✅ Honors meta-tag directives (noindex/nofollow)
✅ Identifies itself with User-Agent
✅ Uses reasonable timeouts
✅ Logs all actions

Before crawling:
1. Check https://yoursite.com/robots.txt
2. Verify crawling is allowed
3. Use descriptive User-Agent
4. Don't overload the server

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 🚀 EXAMPLE SCRIPTS

# Single site
bash examples.sh 1

# Multiple sites
bash examples.sh 2

# Wikipedia (education)
bash examples.sh 3

# GitHub Trending
bash examples.sh 4

# Custom User-Agent
bash examples.sh 6

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## 📚 DOCUMENTATION

SETUP.md       - Full setup guide
USAGE.md       - Complete usage instructions
LOGGING.md     - Logging system documentation
ETHICS.md      - Ethical crawling guidelines
README.md      - API and architecture

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ❓ TROUBLESHOOTING

Problem: "Failed to open JSON file"
Solution: mkdir -p build/output

Problem: Crawler blocked by site
Solution: Increase timeout: --timeout 90

Problem: robots.txt blocks access
Solution: This is correct! Respect these restrictions.

Problem: 403/429 errors
Solution: Site is rate-limiting. Increase timeout or reduce frequency.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ⚖️ LEGAL NOTICE

Before crawling ANY website:
✅ Read Terms of Service
✅ Check robots.txt
✅ Verify crawling is permitted
✅ Use appropriate User-Agent
✅ Don't overload servers
✅ Respect copyright

Failure to do so may result in:
- Account suspension
- Legal action
- Fines up to $100,000+

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💡 QUICK START COMMAND:

cd build && ./crawler --url "https://example.com"

╚════════════════════════════════════════════════════════════════╝
EOF
