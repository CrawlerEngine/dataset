FROM ubuntu:24.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV COMPILER=gcc

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    wget \
    libcurl4-openssl-dev \
    libparquet-dev \
    libparquet0 \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
RUN pip3 install --no-cache-dir \
    pandas \
    pyarrow

# Create app directory
WORKDIR /app

# Copy project files
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY scripts/ ./scripts/

# Make Python scripts executable
RUN chmod +x scripts/*.py

# Build the crawler
RUN mkdir -p build && cd build && \
    cmake .. && \
    make

# Set entrypoint
ENTRYPOINT ["/app/build/crawler"]
CMD []
