#!/bin/bash

# Build and run the crawler
# Usage: ./run.sh [build|clean|examples|help]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
OUTPUT_DIR="$PROJECT_DIR/output"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

echo_error() {
    echo -e "${RED}✗ $1${NC}"
}

echo_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Check dependencies
check_dependencies() {
    echo_info "Checking dependencies..."
    
    local missing=0
    
    if ! command -v cmake &> /dev/null; then
        echo_error "CMake not found"
        missing=1
    fi
    
    if ! command -v pkg-config &> /dev/null; then
        echo_error "pkg-config not found"
        missing=1
    fi
    
    if [[ $missing -eq 1 ]]; then
        echo_error "Missing dependencies. Run: make install-deps"
        return 1
    fi
    
    echo_success "All dependencies found"
    return 0
}

# Build the project
build() {
    echo_info "Building project..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake .. || {
        echo_error "CMake configuration failed"
        return 1
    }
    
    make || {
        echo_error "Build failed"
        return 1
    }
    
    cd "$PROJECT_DIR"
    echo_success "Build completed"
}

# Run the crawler
run() {
    if [[ ! -f "$BUILD_DIR/crawler" ]]; then
        echo_info "Executable not found, building..."
        build || return 1
    fi
    
    mkdir -p "$OUTPUT_DIR"
    
    echo_info "Running crawler..."
    echo ""
    
    cd "$BUILD_DIR"
    ./crawler
    
    cd "$PROJECT_DIR"
    
    if [[ -f "dataset.parquet" ]]; then
        echo ""
        echo_success "Dataset saved to dataset.parquet"
        echo_info "File size: $(du -h dataset.parquet | cut -f1)"
    fi
}

# Run examples
examples() {
    if [[ ! -f "$BUILD_DIR/crawler" ]]; then
        echo_info "Building examples..."
        build || return 1
    fi
    
    echo ""
    echo_success "Available examples:"
    echo "  1 - Simple single URL crawl"
    echo "  2 - Batch URL crawl"
    echo "  3 - Custom headers"
    echo "  4 - Multiple formats (Parquet + CSV)"
    echo "  5 - Error handling"
    echo "  6 - Incremental collection"
    echo ""
}

# Clean build artifacts
clean() {
    echo_info "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    rm -f *.parquet *.csv
    echo_success "Cleaned"
}

# Show help
show_help() {
    cat << EOF
Dataset Crawler - C++ Web Crawler with Parquet Support

Usage: ./run.sh [COMMAND]

Commands:
    build       Build the project
    run         Run the crawler (builds if needed)
    clean       Remove build artifacts
    examples    Show available examples
    help        Show this help message

Examples:
    ./run.sh build          # Just build
    ./run.sh run            # Build and run
    ./run.sh clean          # Clean everything
    ./run.sh examples       # List examples

For more information, see QUICKSTART.md and README.md
EOF
}

# Main logic
main() {
    local command="${1:-run}"
    
    case "$command" in
        build)
            check_dependencies || exit 1
            build
            ;;
        run)
            check_dependencies || exit 1
            run
            ;;
        clean)
            clean
            ;;
        examples)
            examples
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            echo_error "Unknown command: $command"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

main "$@"
