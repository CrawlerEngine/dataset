#!/bin/bash

# Dataset Crawler - Script Examples
# Примеры использования краулера

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     Dataset Crawler - Examples & Usage                     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if crawler is built
if [ ! -f "build/crawler" ]; then
    echo "❌ Crawler not built. Building now..."
    cd build
    cmake ..
    make
    cd ..
fi

# Ensure output directory exists
mkdir -p build/output

echo "Available examples:"
echo ""
echo "1. Crawl single URL (example.com)"
echo "   ./crawler --url 'https://example.com'"
echo ""
echo "2. Crawl multiple URLs"
echo "   ./crawler --urls 'https://example.com,https://example.org'"
echo ""
echo "3. Crawl Wikipedia (educational content)"
echo "   ./crawler --urls 'https://en.wikipedia.org/wiki/Machine_learning,https://en.wikipedia.org/wiki/Artificial_intelligence'"
echo ""
echo "4. Crawl GitHub Trending"
echo "   ./crawler --url 'https://github.com/trending' --timeout 30"
echo ""
echo "5. Crawl ArXiv (scientific papers)"
echo "   ./crawler --url 'https://arxiv.org/list/cs.AI/recent' --timeout 20"
echo ""
echo "6. With custom User-Agent"
echo "   ./crawler --url 'https://example.com' --user-agent 'MyBot/1.0'"
echo ""
echo "7. With increased timeout"
echo "   ./crawler --url 'https://example.com' --timeout 60"
echo ""
echo "8. Using config file"
echo "   ./crawler --config config.json"
echo ""
echo "⚠️  LinkedIn is STRICTLY PROHIBITED!"
echo "   LinkedIn forbids automated crawling in their Terms of Service"
echo "   Use official LinkedIn API instead!"
echo ""

if [ "$1" = "1" ]; then
    echo "Running example 1: Single URL..."
    cd build
    ./crawler --url "https://example.com"
    
elif [ "$1" = "2" ]; then
    echo "Running example 2: Multiple URLs..."
    cd build
    ./crawler --urls "https://example.com,https://example.org"
    
elif [ "$1" = "3" ]; then
    echo "Running example 3: Wikipedia..."
    cd build
    ./crawler --urls "https://en.wikipedia.org/wiki/Machine_learning,https://en.wikipedia.org/wiki/Artificial_intelligence"
    
elif [ "$1" = "4" ]; then
    echo "Running example 4: GitHub Trending..."
    cd build
    ./crawler --url "https://github.com/trending" --timeout 30
    
elif [ "$1" = "5" ]; then
    echo "Running example 5: ArXiv..."
    cd build
    ./crawler --url "https://arxiv.org/list/cs.AI/recent" --timeout 20
    
elif [ "$1" = "6" ]; then
    echo "Running example 6: Custom User-Agent..."
    cd build
    ./crawler --url "https://example.com" --user-agent "MyBot/1.0"
    
elif [ "$1" = "7" ]; then
    echo "Running example 7: Increased timeout..."
    cd build
    ./crawler --url "https://example.com" --timeout 60
    
elif [ "$1" = "8" ]; then
    echo "Running example 8: Config file..."
    cd build
    ./crawler --config ../config.json
    
elif [ "$1" = "" ]; then
    echo "💡 Run with example number: ./examples.sh <number>"
    echo "   Example: ./examples.sh 1"
    
else
    echo "❌ Unknown example: $1"
    echo "   Use numbers 1-8"
fi

if [ -f "build/output/dataset.json" ]; then
    echo ""
    echo "✅ Output files created:"
    echo "   📄 build/output/dataset.json"
    echo "   📊 build/output/dataset.csv"
    echo ""
    echo "Preview (first few lines of JSON):"
    head -20 "build/output/dataset.json"
fi
