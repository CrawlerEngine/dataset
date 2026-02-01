#!/bin/bash
# CHEATSHEET.md - Quick Reference

# Installation & Setup
./install_dependencies.sh              # Install all dependencies
make install-deps                      # Alternative with Makefile

# Building
mkdir build && cd build && cmake .. && make  # Manual build
make build                             # Using Makefile
./run.sh build                        # Using run script

# Running
./build/crawler                        # Direct execution
make run                               # Using Makefile  
./run.sh run                          # Using run script

# Docker
docker build -t crawler .              # Build Docker image
docker run crawler                     # Run in container
docker-compose up                      # Run with Docker Compose

# Cleaning
rm -rf build                           # Delete build directory
make clean                             # Using Makefile
./run.sh clean                        # Using run script

# Python Utilities
python3 scripts/parquet_utils.py info dataset.parquet              # Show info
python3 scripts/parquet_utils.py to-csv dataset.parquet out.csv   # Convert to CSV
python3 scripts/parquet_utils.py to-json dataset.parquet out.json # Convert to JSON
python3 scripts/parquet_utils.py merge *.parquet -o merged.parquet # Merge files
python3 scripts/parquet_utils.py filter dataset.parquet 200 -o success.parquet # Filter
python3 scripts/parquet_utils.py sample dataset.parquet 100 -o sample.parquet # Sample

# Data Analysis (Python)
python3 -c "import pandas as pd; df=pd.read_parquet('dataset.parquet'); print(df.info())"
python3 -c "import pandas as pd; df=pd.read_parquet('dataset.parquet'); print(df['status_code'].value_counts())"

# Help
./run.sh help                          # Show run.sh options
make help                              # Show Makefile options
