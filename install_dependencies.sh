#!/bin/bash

# Installation script for dependencies
set -e

echo "Installing dependencies for Dataset Crawler..."

# Update package list
sudo apt-get update

# Install CMake
echo "Installing CMake..."
sudo apt-get install -y cmake

# Install build essentials
echo "Installing build tools..."
sudo apt-get install -y build-essential git

# Install CURL
echo "Installing libcurl..."
sudo apt-get install -y libcurl4-openssl-dev

# Install Apache Arrow and Parquet
echo "Installing Apache Arrow and Parquet..."
sudo apt-get install -y libparquet-dev libparquet0

echo ""
echo "✓ All dependencies installed successfully!"
echo ""
echo "Next steps:"
echo "1. Create build directory: mkdir build && cd build"
echo "2. Configure: cmake .."
echo "3. Build: make"
echo "4. Run: ./crawler"
