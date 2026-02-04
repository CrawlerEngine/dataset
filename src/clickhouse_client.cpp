#include "clickhouse_client.h"
#include "logger.h"

#include <curl/curl.h>
#include <sstream>

namespace {

std::string escape_json(std::string value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}

} // namespace

ClickHouseClient::ClickHouseClient(const ClickHouseConfig& config)
    : config_(config) {}

bool ClickHouseClient::is_enabled() const {
    return config_.enabled;
}

bool ClickHouseClient::insert_request_metric(const ClickHouseRequestMetric& metric) const {
    if (!config_.enabled) {
        return false;
    }

    std::ostringstream payload;
    payload << "{\"url\":\"" << escape_json(metric.url) << "\","
            << "\"status_code\":" << metric.status_code << ","
            << "\"duration_ms\":" << metric.duration_ms << ","
            << "\"bytes\":" << metric.bytes << ","
            << "\"content_type\":\"" << escape_json(metric.content_type) << "\","
            << "\"timestamp\":\"" << escape_json(metric.timestamp) << "\","
            << "\"success\":" << (metric.success ? "true" : "false") << ","
            << "\"error_message\":\"" << escape_json(metric.error_message) << "\"}\n";

    std::ostringstream query;
    query << "INSERT INTO " << config_.database << "." << config_.metrics_table
          << " FORMAT JSONEachRow";

    return perform_insert(query.str(), payload.str());
}

bool ClickHouseClient::insert_link_edge(const ClickHouseLinkEdge& edge) const {
    if (!config_.enabled) {
        return false;
    }

    std::ostringstream payload;
    payload << "{\"from_url\":\"" << escape_json(edge.from_url) << "\","
            << "\"to_url\":\"" << escape_json(edge.to_url) << "\","
            << "\"discovered_at\":\"" << escape_json(edge.discovered_at) << "\"}\n";

    std::ostringstream query;
    query << "INSERT INTO " << config_.database << "." << config_.link_graph_table
          << " FORMAT JSONEachRow";

    return perform_insert(query.str(), payload.str());
}

bool ClickHouseClient::perform_insert(const std::string& query, const std::string& payload) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        log_error("ClickHouse: Failed to initialize CURL");
        return false;
    }

    std::string url = build_endpoint_url(query);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);

    if (!config_.user.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, config_.user.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, config_.password.c_str());
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::ostringstream error_msg;
        error_msg << "ClickHouse: insert failed: " << curl_easy_strerror(res);
        log_warn(error_msg.str());
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);
    return true;
}

std::string ClickHouseClient::build_endpoint_url(const std::string& query) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return config_.endpoint;
    }

    char* encoded = curl_easy_escape(curl, query.c_str(), static_cast<int>(query.size()));
    std::ostringstream url;
    url << config_.endpoint;
    if (!config_.endpoint.empty() && config_.endpoint.back() != '/') {
        url << "/";
    }
    url << "?query=" << (encoded ? encoded : "");

    if (encoded) {
        curl_free(encoded);
    }
    curl_easy_cleanup(curl);
    return url.str();
}
