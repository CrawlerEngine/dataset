# robots.txt User-Agent Priority Matching

## Overview

The crawler now implements proper robots.txt User-Agent priority matching according to [Google's robots.txt specification](https://developers.google.com/search/docs/crawling-indexing/robots-txt). This ensures that specific User-Agent rules take precedence over generic wildcard rules.

## User-Agent Specificity Rules

The implementation respects the following specificity order (highest to lowest):

1. **Exact Match (Specificity 3)**: "googlebot" matches exactly
   - `User-agent: googlebot`

2. **Pattern Match (Specificity 2)**: Complex patterns with special characters
   - `User-agent: googlebot*`
   - `User-agent: crawler/1.0`

3. **Wildcard Match (Specificity 1)**: Matches any user agent
   - `User-agent: *`

## Key Features

### 1. User-Agent Normalization
Version suffixes are automatically removed and normalized:
- `googlebot/1.2` → `googlebot`
- `bingbot*` → `bingbot`
- `crawler/2.0` → `crawler`

### 2. Rule Combination
- **Specific rules combine with each other**: Multiple groups for the same User-Agent are combined (e.g., two different "googlebot" rule groups both apply)
- **Wildcard rules are independent**: If any specific rule matches, wildcard rules are ignored
- **Fallback behavior**: If no specific rules match, wildcard rules apply

### 3. Allow Precedence
The `Allow` directive takes precedence over `Disallow`:
```
User-agent: googlebot
Disallow: /private
Allow: /private/public
```
In this case, `/private/public` is allowed even though `/private` is disallowed.

### 4. Path Matching
Paths are matched using prefix matching:
- `Disallow: /admin` blocks `/admin`, `/admin/`, `/admin/users`, etc.
- `Disallow: /` blocks everything
- `Allow: /private/public` allows `/private/public` and anything under it

## API Usage

### Parsing robots.txt

```cpp
WebCrawler crawler("MyBot/1.0");

// Parse robots.txt content
std::vector<RobotRule> rules = crawler.parse_robots_txt(
    "example.com",
    robots_content
);

// Access parsed rules
for (const auto& rule : rules) {
    std::cout << "Specificity: " << rule.specificity << std::endl;
    
    for (const auto& agent : rule.user_agents) {
        std::cout << "  User-agent: " << agent << std::endl;
    }
    
    for (const auto& disallow : rule.disallows) {
        std::cout << "  Disallow: " << disallow << std::endl;
    }
    
    for (const auto& allow : rule.allows) {
        std::cout << "  Allow: " << allow << std::endl;
    }
}
```

### Checking Path Access

The crawler automatically uses these rules when checking `robots.txt` compliance:

```cpp
// fetch() automatically respects robots.txt rules
DataRecord record = crawler.fetch("https://example.com/private/data");

// Blocked paths will have was_allowed = false
if (!record.was_allowed) {
    std::cout << "Page blocked by robots.txt" << std::endl;
}
```

### Manual User-Agent Matching

```cpp
// Check if a rule matches your User-Agent
bool matches = crawler.matches_user_agent("googlebot", "MyBot/1.0");

// Normalize User-Agent strings
std::string normalized = crawler.normalize_user_agent("googlebot/1.2");
// Result: "googlebot"
```

## Example: robots.txt with Multiple Rules

```
# Specific rules for Googlebot (highest priority)
User-agent: googlebot
Disallow: /temp
Allow: /temp/cache

# Rules for Bingbot
User-agent: bingbot
Disallow: /tmp

# Fallback rules for all other bots
User-agent: *
Disallow: /private
Disallow: /admin
```

### Matching Results

For `googlebot/1.2`:
- Uses googlebot rule (specificity 3 - exact match)
- `/temp` is disallowed, but `/temp/cache` is allowed
- `/private` and `/admin` rules don't apply

For `bingbot`:
- Uses bingbot rule (specificity 3 - exact match)
- `/tmp` is disallowed
- `/private` and `/admin` rules don't apply

For `unknownbot`:
- Uses wildcard rule (specificity 1)
- `/private` and `/admin` are disallowed

## Implementation Details

### RobotRule Structure

```cpp
struct RobotRule {
    std::vector<std::string> user_agents;    // User-Agents in this group
    std::vector<std::string> disallows;      // Disallow: paths
    std::vector<std::string> allows;         // Allow: paths
    int specificity = 0;                     // 1=wildcard, 2=pattern, 3=exact
};
```

### Caching

Parsed robots.txt rules are cached per domain to improve performance:

```cpp
// First access fetches and parses robots.txt
bool allowed1 = crawler.check_robots_txt("https://example.com/path1");

// Second access uses cached rules
bool allowed2 = crawler.check_robots_txt("https://example.com/path2");
```

## Testing

Run the test suite:

```bash
./build/test_robots_ua_priority
```

This tests:
- User-Agent normalization (removing version/wildcard suffixes)
- User-Agent matching (exact, wildcard, case-insensitive)
- Specificity calculation
- Allow precedence over Disallow
- Multiple rule group combination
- Wildcard fallback
- Root disallow handling

## References

- [Google robots.txt Specification](https://developers.google.com/search/docs/crawling-indexing/robots-txt)
- [RFC 9309 - The `robots.txt` File](https://www.rfc-editor.org/rfc/rfc9309.html)

## Performance Notes

- User-Agent rules are parsed once per domain and cached
- Path matching uses O(n) prefix matching
- Overall complexity is O(number_of_rules × number_of_allows/disallows)

## Limitations

- Regex patterns in User-Agent strings are not supported (only exact and wildcard matching)
- Comments are stripped during parsing
- Case-insensitive matching is performed
- Path matching is prefix-based, not full regex
