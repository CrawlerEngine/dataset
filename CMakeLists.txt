cmake_minimum_required(VERSION 3.20)
project(DatasetCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(RocksDB REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add source files
set(SOURCES
    src/main.cpp
    src/crawler.cpp
    src/dataset_writer.cpp
    src/logger.cpp
    src/config_loader.cpp
    src/text_extractor.cpp
    src/rocksdb_manager.cpp
    src/raw_socket_http.cpp
)

# Create executable
add_executable(crawler ${SOURCES})

# Link libraries
target_link_libraries(crawler
    PRIVATE
    CURL::libcurl
    rocksdb
    gumbo
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Compiler flags
if(MSVC)
    target_compile_options(crawler PRIVATE /W4)
else()
    target_compile_options(crawler PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add test executable for robots.txt User-Agent priority
add_executable(test_robots_ua_priority
    test_robots_ua_priority.cpp
    src/crawler.cpp
    src/dataset_writer.cpp
    src/logger.cpp
    src/config_loader.cpp
    src/text_extractor.cpp
    src/rocksdb_manager.cpp
    src/raw_socket_http.cpp
)

target_link_libraries(test_robots_ua_priority
    PRIVATE
    CURL::libcurl
    rocksdb
    gumbo
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(test_robots_ua_priority PRIVATE /W4)
else()
    target_compile_options(test_robots_ua_priority PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add integration test for robots.txt
add_executable(test_robots_integration
    test_robots_integration.cpp
    src/crawler.cpp
    src/dataset_writer.cpp
    src/logger.cpp
    src/config_loader.cpp
    src/text_extractor.cpp
    src/rocksdb_manager.cpp
    src/raw_socket_http.cpp
)

target_link_libraries(test_robots_integration
    PRIVATE
    CURL::libcurl
    rocksdb
    gumbo
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(test_robots_integration PRIVATE /W4)
else()
    target_compile_options(test_robots_integration PRIVATE -Wall -Wextra -Wpedantic)
endif()
# Add wildcard test for robots.txt
add_executable(test_robots_wildcard
    test_robots_wildcard.cpp
    src/crawler.cpp
    src/dataset_writer.cpp
    src/logger.cpp
    src/config_loader.cpp
    src/text_extractor.cpp
    src/rocksdb_manager.cpp
    src/raw_socket_http.cpp
)

target_link_libraries(test_robots_wildcard
    PRIVATE
    CURL::libcurl
    rocksdb
    gumbo
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(test_robots_wildcard PRIVATE /W4)
else()
    target_compile_options(test_robots_wildcard PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add RocksDB test
add_executable(test_rocksdb
    test_rocksdb.cpp
    src/rocksdb_manager.cpp
    src/logger.cpp
)

target_link_libraries(test_rocksdb
    PRIVATE
    rocksdb
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(test_rocksdb PRIVATE /W4)
else()
    target_compile_options(test_rocksdb PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add TextExtractor test
add_executable(test_text_extractor
    test_text_extractor.cpp
    src/text_extractor.cpp
    src/logger.cpp
)

target_link_libraries(test_text_extractor
    PRIVATE
    gumbo
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(test_text_extractor PRIVATE /W4)
else()
    target_compile_options(test_text_extractor PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(test_robots_wildcard
    PRIVATE
    CURL::libcurl
    rocksdb
    gumbo
)

if(MSVC)
    target_compile_options(test_robots_wildcard PRIVATE /W4)
else()
    target_compile_options(test_robots_wildcard PRIVATE -Wall -Wextra -Wpedantic)
endif()
