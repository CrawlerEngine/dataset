# Enhanced Logging for URL Parsing, Redirects, and robots.txt Errors

## Overview

Improved logging system with more detailed and user-friendly error messages for three critical scenarios:

1. **URL Parsing Errors** - When the crawler encounters invalid or malformed URLs
2. **HTTP Redirects** - When the crawler follows HTTP redirects (301, 302, etc.)
3. **robots.txt Fetch Failures** - When the crawler fails to fetch or parse robots.txt

## Log Messages

### 1. URL Parsing Errors

**When**: The crawler encounters an invalid or malformed URL during crawling

**Format**:
```
YYYY-MM-DDTHH:MM:SS.sssZ WARN  Failed to parse URL: <error_description>
```

**Examples**:
```
2026-01-25T07:04:36.526Z WARN  Failed to parse URL: Unsupported URL scheme
2026-01-25T07:04:37.123Z WARN  Failed to parse URL: malformed URL
```

**When it appears**:
- Invalid URL schemes (ftp://, gopher://, etc. if not supported)
- Malformed URLs with syntax errors
- URLs that libcurl cannot parse

**Code location**: `src/crawler.cpp` - `fetch_html()` method

### 2. HTTP Redirects

**When**: The crawler successfully follows an HTTP redirect (301, 302, 303, 307, 308)

**Format**:
```
YYYY-MM-DDTHH:MM:SS.sssZ WARN  The start URL "<original_url>" has been redirected to "<new_url>". Processing the redirected URL...
```

**Examples**:
```
2026-01-25T07:04:34.539Z WARN  The start URL "https://flask.palletsprojects.com/" has been redirected to "https://flask.palletsprojects.com/en/stable/". Processing the redirected URL...
2026-01-25T07:05:12.845Z WARN  The start URL "http://example.com" has been redirected to "https://example.com". Processing the redirected URL...
```

**When it appears**:
- Website uses 301 (permanent) redirects
- Website uses 302 (temporary) redirects
- Website enforces HTTPS by redirecting HTTP to HTTPS
- Website redirects to a different path or domain

**Code location**: `src/crawler.cpp` - `fetch_html()` method

**Note**: The crawler automatically follows redirects (up to CURL's default limit of 5) and reports them for transparency.

### 3. robots.txt Fetch Failures

**When**: The crawler fails to fetch or parse robots.txt file (except 404)

**Format**:
```
YYYY-MM-DDTHH:MM:SS.sssZ WARN  Failed to fetch robots.txt for request <full_url>
```

**Examples**:
```
2026-01-25T07:04:51.514Z WARN  Failed to fetch robots.txt for request https://developer.android.com/develop/background-work
2026-01-25T07:05:23.342Z WARN  Failed to fetch robots.txt for request https://example.com/page
```

**When it appears**:
- robots.txt server returns 5xx error (server error)
- Network timeout while fetching robots.txt
- SSL/TLS certificate errors
- Other HTTP errors (but not 404 - 404 means no robots.txt, which is normal)

**Code location**: `src/crawler.cpp` - `check_robots_txt()` method

**Note**: 404 responses are **not logged** because they are expected and normal (no robots.txt file exists).

## Log Levels

The logging uses three levels:

| Level | Color | Usage | Examples |
|-------|-------|-------|----------|
| **INFO** | Green | Successful operations | URL fetched successfully, links extracted |
| **WARN** | Yellow | Non-fatal issues that require attention | URL parsing errors, redirects, robots.txt failures |
| **ERROR** | Red | Fatal errors that stop processing | CURL errors, file write errors |

## Configuration

Logging is enabled by default in the crawler and cannot be disabled via configuration. All logs go to standard output with ISO 8601 timestamps.

**Timestamp format**: `YYYY-MM-DDTHH:MM:SS.sssZ`
- Year-Month-Day
- Time with milliseconds
- Z suffix for UTC/Zulu time

## Examples in Context

### Complete Crawl with All Three Log Types

```
2026-01-25T07:04:32.100Z INFO   === Dataset Crawler for AI ===
2026-01-25T07:04:32.200Z INFO   Configuration: 2 URLs, timeout: 30s, robots.txt: YES, meta-tags: YES
2026-01-25T07:04:32.250Z INFO   Starting the crawler.

2026-01-25T07:04:34.539Z WARN   The start URL "https://flask.palletsprojects.com/" has been redirected to "https://flask.palletsprojects.com/en/stable/". Processing the redirected URL...
2026-01-25T07:04:35.123Z INFO   https://flask.palletsprojects.com/en/stable/ [200]
2026-01-25T07:04:35.124Z INFO   Enqueued 15 new links on https://flask.palletsprojects.com/en/stable/

2026-01-25T07:04:36.526Z WARN   Failed to parse URL: Unsupported URL scheme
2026-01-25T07:04:36.527Z WARN   https://invalid://example.com [failed]

2026-01-25T07:04:51.514Z WARN   Failed to fetch robots.txt for request https://developer.android.com/develop/background-work
2026-01-25T07:04:52.123Z INFO   https://developer.android.com/develop/background-work [200]

2026-01-25T07:05:45.678Z INFO   Crawling completed. Fetched: 18 records, Blocked by robots.txt: 2, Blocked by noindex: 1, Skipped by size: 0
```

## Testing the New Logging

To test the new logging in action:

1. **Test URL parsing error**: Add a malformed URL to config
2. **Test redirect logging**: Use a URL that redirects (e.g., example.com → www.example.com)
3. **Test robots.txt logging**: Use a domain with robots.txt fetch issues

### Example Configuration for Testing

```json
{
  "urls": [
    "https://flask.palletsprojects.com/",
    "invalid://malformed",
    "https://developer.android.com/develop/background-work"
  ],
  "respect_robots_txt": true,
  "timeout": 30
}
```

## Integration with Existing Logging

All new log messages integrate seamlessly with the existing logging system:

- Uses the same `log_warn()`, `log_info()`, `log_error()` functions
- Maintains consistent timestamp format (ISO 8601)
- Supports the same color-coded output
- Respects the same logging configuration

## Code Implementation Details

### URL Parsing Errors
- Detected when CURL returns an error in `fetch_html()`
- Checks if error message contains "Unsupported", "Invalid", or "malformed"
- Logs as WARN level (yellow) for visibility

### Redirect Detection
- Uses CURL's `CURLINFO_EFFECTIVE_URL` to get final URL
- Compares initial URL with effective URL
- Automatically logs when they differ
- Only logs if CURL successfully follows redirect

### robots.txt Failures
- Checked in `check_robots_txt()` method
- Only logs on actual errors (HTTP 5xx, timeouts, etc.)
- Ignores 404 responses (normal - no robots.txt exists)
- Includes the full request URL for context

## Performance Impact

- Minimal performance impact: only string comparisons
- No additional network calls
- Information already gathered by CURL, just logged
- Logging operations are asynchronous in color-coded output

## Future Enhancements

Potential improvements:
- [ ] Log redirect chain (e.g., A → B → C)
- [ ] Track robots.txt error statistics
- [ ] Warn about excessive redirects (potential loops)
- [ ] Log robots.txt parsing errors in detail
- [ ] Add debug mode with more verbose logging

## Compliance

- Complies with logging standards
- Uses standard timestamp format (ISO 8601)
- Consistent with crawler's existing logging system
- User-friendly messages for debugging
