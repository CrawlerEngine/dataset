#ifndef CLICKHOUSE_CLIENT_H
#define CLICKHOUSE_CLIENT_H

#include <string>

struct ClickHouseConfig {
    bool enabled = false;
    std::string endpoint = "http://localhost:8123";
    std::string database = "default";
    std::string metrics_table = "crawler_metrics";
    std::string link_graph_table = "crawler_link_graph";
    std::string user;
    std::string password;
    int timeout_seconds = 5;
};

struct ClickHouseRequestMetric {
    std::string url;
    int status_code = 0;
    long duration_ms = 0;
    size_t bytes = 0;
    std::string content_type;
    std::string timestamp;
    bool success = false;
    std::string error_message;
};

struct ClickHouseLinkEdge {
    std::string from_url;
    std::string to_url;
    std::string discovered_at;
};

class ClickHouseClient {
public:
    explicit ClickHouseClient(const ClickHouseConfig& config);

    bool is_enabled() const;
    bool insert_request_metric(const ClickHouseRequestMetric& metric) const;
    bool insert_link_edge(const ClickHouseLinkEdge& edge) const;

private:
    bool perform_insert(const std::string& query, const std::string& payload) const;
    std::string build_endpoint_url(const std::string& query) const;

    ClickHouseConfig config_;
};

#endif // CLICKHOUSE_CLIENT_H
