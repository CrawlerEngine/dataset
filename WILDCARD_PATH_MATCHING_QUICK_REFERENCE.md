# Wildcard Path Matching - Quick Reference

## Pattern Examples

| Pattern | Examples That Match | Examples That DON'T Match |
|---------|---------------------|---------------------------|
| `/fish` | `/fish`, `/fish.html`, `/fishheads` | `/catfish` |
| `/fish/` | `/fish/salmon.html`, `/fish/` | `/fish`, `/fish.html` |
| `/*.php` | `/index.php`, `/a/b.php`, `/test.php` | `/test.html`, `/php` |
| `/*.php$` | `/index.php`, `/test.php` | `/index.php.bak`, `/test.html` |
| `/admin/*` | `/admin/`, `/admin/page.html` | `/admin`, `/admins/` |
| `/admin/*$` | `/admin/`, `/admin/secret` | `/admin`, `/admin/secret/more` |
| `/fish*.php` | `/fish.php`, `/fish123.php`, `/fishheads.php` | `/catfish.php`, `/fish.html` |
| `/*.php*` | `/index.php`, `/index.php.bak`, `/file.php5` | `/file.html`, `/test` |

## Key Rules

1. **`*` = any characters** (0 or more)
   - `*.html` matches `/index.html`, `/a/b.html`, `/very/long/path.html`

2. **`$` = end of URL** (must be at end of pattern)
   - `.php$` means exactly ending with `.php`, not `.php.bak`

3. **Longest match wins**
   - If `/` and `/admin` both match, `/admin` is used (it's longer)

4. **Allow beats Disallow** (when equal length)
   - If both allow and disallow have same specificity, Allow wins

5. **Case-sensitive**
   - `/Fish` and `/fish` are different paths

## Common Patterns

```
# Block all PHP files
Disallow: /*.php

# Block only in specific directory
Disallow: /private/*

# Block all admin paths (except public ones)
Disallow: /admin/*
Allow: /admin/public/*

# Block files with certain extensions
Disallow: /*.pdf$
Disallow: /*.zip$

# Block query parameters (use prefix without $)
Disallow: /search?
```

## Code Usage

```cpp
WebCrawler crawler("mybot");

// Create rules with wildcards
RobotRule rule;
rule.user_agents = {"*"};
rule.disallows = {"/*.php", "/admin/*"};
rule.allows = {"/admin/public/*"};
rule.specificity = 1;

std::vector<RobotRule> rules = {rule};

// Check if path is allowed
bool allowed = crawler.is_path_allowed(rules, "/index.php");  // false
bool allowed2 = crawler.is_path_allowed(rules, "/index.html"); // true
bool allowed3 = crawler.is_path_allowed(rules, "/admin/secret"); // false
bool allowed4 = crawler.is_path_allowed(rules, "/admin/public/page.html"); // true
```

## Implementation Methods

### `match_path_pattern(pattern, path)`
- **Input**: Pattern (e.g., `/*.php`), Path (e.g., `/index.php`)
- **Output**: `true` if matches, `false` otherwise
- **Uses**: Regex matching with wildcard conversion

### `is_path_allowed(rules, path)`
- **Input**: Vector of RobotRules, Path to check
- **Output**: `true` if path is allowed, `false` if disallowed
- **Logic**: Finds longest matching rule and applies it

## Testing

Run the wildcard test suite:
```bash
cd build
./test_robots_wildcard
```

Expected output:
```
RESULTS: 37 tests passed, 0 failed
✓ All wildcard tests passed!
```

## Version Info

- **Feature**: Advanced Path Matching with Wildcards
- **Specification**: Google robots.txt spec
- **Tests**: 37 dedicated wildcard tests + 37 original tests
- **Status**: Fully implemented and tested
- **Backward Compatible**: Yes, 100%

---

For more details, see [WILDCARD_PATH_MATCHING.md](WILDCARD_PATH_MATCHING.md)
