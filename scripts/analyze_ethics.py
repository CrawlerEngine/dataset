#!/usr/bin/env python3
"""
Script to analyze ethical crawling statistics from dataset
"""

import json
import sys
from pathlib import Path


def analyze_dataset(json_file):
    """Analyze crawled dataset and ethics statistics"""
    
    if not Path(json_file).exists():
        print(f"Error: {json_file} not found")
        return
    
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    print("\n=== Dataset Analysis ===\n")
    
    # Basic stats
    total = len(data)
    print(f"Total records: {total}")
    
    # Status code distribution
    status_codes = {}
    for item in data:
        code = item['status_code']
        status_codes[code] = status_codes.get(code, 0) + 1
    
    print("\nHTTP Status Distribution:")
    for code in sorted(status_codes.keys()):
        count = status_codes[code]
        percentage = (count / total * 100) if total > 0 else 0
        print(f"  {code}: {count} ({percentage:.1f}%)")
    
    # Domain distribution
    domains = {}
    for item in data:
        url = item['url']
        # Extract domain
        start = url.find("://") + 3
        end = url.find("/", start)
        if end == -1:
            end = len(url)
        domain = url[start:end]
        domains[domain] = domains.get(domain, 0) + 1
    
    print("\nDomains crawled:")
    for domain in sorted(domains.keys(), key=lambda x: domains[x], reverse=True):
        count = domains[domain]
        print(f"  {domain}: {count} pages")
    
    # Content size stats
    sizes = [item['content_length'] for item in data]
    if sizes:
        avg_size = sum(sizes) / len(sizes)
        max_size = max(sizes)
        min_size = min(sizes)
        total_size = sum(sizes)
        
        print(f"\nContent Size Statistics:")
        print(f"  Average: {avg_size:,.0f} bytes")
        print(f"  Min: {min_size:,} bytes")
        print(f"  Max: {max_size:,} bytes")
        print(f"  Total: {total_size:,} bytes ({total_size / 1024 / 1024:.2f} MB)")
    
    print("\n" + "="*50 + "\n")


if __name__ == "__main__":
    json_file = "dataset.json" if len(sys.argv) < 2 else sys.argv[1]
    analyze_dataset(json_file)
