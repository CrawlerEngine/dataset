#!/bin/bash

echo "=== Dataset Crawler v2.0 - Feature Test ==="
echo ""

# Test 1: Basic crawling
echo "Test 1: Basic crawling with statistics..."
rm -f output/dataset.json
./build/crawler --url https://example.com --max-depth 1 2>&1 | grep -E "Statistics:|Fetched:|Skipped"

echo ""
echo "Test 2: Output file created and valid JSON..."
if [ -f output/dataset.json ]; then
    echo "✓ Output file exists"
    python3 -m json.tool output/dataset.json > /dev/null 2>&1 && echo "✓ Valid JSON format" || echo "✗ Invalid JSON"
else
    echo "✗ Output file missing"
fi

echo ""
echo "Test 3: Verify content_length field..."
if cat output/dataset.json | grep -q "content_length"; then
    echo "✓ content_length field present"
else
    echo "✗ content_length field missing"
fi

echo ""
echo "Test 4: Check compiled symbols..."
echo "Checking for new methods in binary:"
nm /workspaces/dataset/build/crawler | grep "_ZN10WebCrawler" | grep -c "get_\|fetch_\|extract_" | xargs echo "✓ Methods found:"

echo ""
echo "=== All Tests Complete ==="
