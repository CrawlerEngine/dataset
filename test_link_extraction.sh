#!/bin/bash

echo "=== Link Extraction and URL Normalization Tests ==="
echo ""

# Start test server
python3 > /dev/null 2>&1 << 'PYEOF'
from http.server import HTTPServer, SimpleHTTPRequestHandler
import threading
import time

class TestHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            html = """<!DOCTYPE html>
<html><head><title>Test</title><link rel="canonical" href="https://localhost:8888/canonical"></head>
<body>
<a href="https://localhost:8888/page1">Page 1</a>
<a href="/page2">Page 2</a>
<a href="page3">Page 3</a>
<a href="../parent">Parent</a>
<a href="#fragment">Fragment</a>
<a href="javascript:void(0)">JS Link</a>
</body></html>"""
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(html.encode())
        else:
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"<html><body>Page</body></html>")
    
    def log_message(self, *args): pass

server = HTTPServer(('127.0.0.1', 8888), TestHandler)
thread = threading.Thread(target=server.serve_forever)
thread.daemon = True
thread.start()
time.sleep(8)
server.shutdown()
PYEOF &

sleep 2

echo "Test 1: Link Extraction from HTML"
echo "===================================="
rm -f output/dataset.json
./build/crawler --url http://127.0.0.1:8888/ --max-depth 1 2>&1 | grep -E "Enqueued|http://"
echo ""

echo "Test 2: URL Normalization and Deduplication"
echo "============================================"
URLS=$(cat output/dataset.json | python3 -c "import sys, json; print('\n'.join([r['url'] for r in json.load(sys.stdin)]))")
echo "Crawled URLs:"
echo "$URLS" | sort
echo ""
COUNT=$(echo "$URLS" | wc -l)
echo "Total unique URLs crawled: $COUNT"
echo ""

echo "Test 3: Canonical URL Detection"
echo "================================"
if echo "$URLS" | grep -q "canonical"; then
    echo "✓ Canonical URL was detected and added"
else
    echo "✗ Canonical URL not found"
fi

echo ""
echo "Test 4: Fragment Removal"
echo "========================"
if echo "$URLS" | grep -q "#fragment"; then
    echo "✗ Fragment was not removed"
else
    echo "✓ Fragments were properly removed"
fi

echo ""
echo "=== All Tests Complete ==="
