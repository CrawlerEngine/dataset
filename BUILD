package(default_visibility = ["//visibility:public"])

# Main executable - crawler
cc_binary(
    name = "crawler",
    srcs = ["src/main.cpp"],
    deps = [
        ":crawler_lib",
    ],
    copts = ["-std=c++17"],
)

# Crawler library
cc_library(
    name = "crawler_lib",
    hdrs = glob(["include/**/*.h"]),
    srcs = glob(["src/**/*.cpp"], exclude = ["src/main.cpp"]),
    includes = ["include"],
    copts = ["-std=c++17"],
    linkopts = [
        "-lrocksdb",
        "-lgumbo",
        "-lcurl",
        "-lssl",
        "-lcrypto",
        "-lz",
    ],
)

# Tests target (aggregates all tests)
test_suite(
    name = "all_tests",
    tests = [
        ":rocksdb_test",
        ":text_extractor_test",
        ":robots_ua_priority_test",
        ":robots_integration_test",
        ":robots_wildcard_test",
    ],
)

# RocksDB Manager Test
cc_test(
    name = "rocksdb_test",
    srcs = ["tests/rocksdb_test.cc"],
    copts = ["-std=c++17"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)

# Text Extractor Test
cc_test(
    name = "text_extractor_test",
    srcs = ["tests/text_extractor_test.cc"],
    copts = ["-std=c++17"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)

# Robots UA Priority Test
cc_test(
    name = "robots_ua_priority_test",
    srcs = ["tests/robots_ua_priority_test.cc"],
    copts = ["-std=c++17"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)

# Robots Integration Test
cc_test(
    name = "robots_integration_test",
    srcs = ["tests/robots_integration_test.cc"],
    copts = ["-std=c++17"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)

# Robots Wildcard Path Matching Test
cc_test(
    name = "robots_wildcard_test",
    srcs = ["tests/robots_wildcard_test.cc"],
    copts = ["-std=c++17"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)
