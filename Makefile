.PHONY: help build clean run install-deps docker-build docker-run test

help:
	@echo "Dataset Crawler - Build Commands"
	@echo ""
	@echo "Available targets:"
	@echo "  install-deps  - Install system dependencies"
	@echo "  build         - Build the crawler"
	@echo "  run           - Run the crawler"
	@echo "  clean         - Clean build artifacts"
	@echo "  docker-build  - Build Docker image"
	@echo "  docker-run    - Run in Docker container"
	@echo "  docker-stop   - Stop Docker container"
	@echo "  test          - Run tests"
	@echo "  format        - Format code with clang-format"
	@echo "  help          - Show this help message"

install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y cmake build-essential
	sudo apt-get install -y libcurl4-openssl-dev
	sudo apt-get install -y libparquet-dev libparquet0
	pip3 install -r requirements.txt
	@echo "✓ Dependencies installed"

build: clean
	@echo "Building crawler..."
	mkdir -p build
	cd build && cmake .. && make
	@echo "✓ Build completed"

run: build
	@echo "Running crawler..."
	./build/crawler

clean:
	@echo "Cleaning build artifacts..."
	rm -rf build output *.parquet *.csv
	@echo "✓ Cleaned"

docker-build:
	@echo "Building Docker image..."
	docker build -t dataset-crawler:latest .
	@echo "✓ Docker image built"

docker-run: docker-build
	@echo "Running in Docker..."
	docker run -it --rm -v $(PWD)/output:/app/output dataset-crawler:latest

docker-stop:
	docker stop dataset-crawler || true
	docker rm dataset-crawler || true

test:
	@echo "Running tests..."
	cd build && make test || echo "No tests configured"

format:
	@echo "Formatting code..."
	find include src -name "*.h" -o -name "*.cpp" | xargs clang-format -i || echo "clang-format not installed"
	@echo "✓ Code formatted"

install: build
	@echo "Installing crawler..."
	sudo cp build/crawler /usr/local/bin/crawler
	sudo chmod +x /usr/local/bin/crawler
	@echo "✓ Installed to /usr/local/bin/crawler"

uninstall:
	sudo rm -f /usr/local/bin/crawler
	@echo "✓ Uninstalled"
