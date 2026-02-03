#include "config_loader.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

CrawlerConfig ConfigLoader::load(const std::string& filepath) {
    try {
        std::string json_str = read_file(filepath);
        CrawlerConfig config = parse_json(json_str);
        
        std::ostringstream msg;
        msg << "Loaded configuration from " << filepath 
            << " with " << config.urls.size() << " URLs";
        log_info(msg.str());
        
        return config;
    } catch (const std::exception& e) {
        std::ostringstream error_msg;
        error_msg << "Failed to load configuration: " << e.what();
        log_error(error_msg.str());
        throw;
    }
}

CrawlerConfig ConfigLoader::from_command_line(int argc, char* argv[]) {
    CrawlerConfig config = get_default();
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--config" && i + 1 < argc) {
            return load(argv[i + 1]);
        }
        if (arg == "--url" && i + 1 < argc) {
            config.urls.clear();
            config.urls.push_back(argv[i + 1]);
        }
        if (arg == "--urls" && i + 1 < argc) {
            // Multiple URLs: --urls "url1,url2,url3"
            std::string urls_str = argv[i + 1];
            std::istringstream iss(urls_str);
            std::string url;
            while (std::getline(iss, url, ',')) {
                // Trim whitespace
                url.erase(0, url.find_first_not_of(" \t"));
                url.erase(url.find_last_not_of(" \t") + 1);
                if (!url.empty()) {
                    config.urls.push_back(url);
                }
            }
        }
        if (arg == "--timeout" && i + 1 < argc) {
            config.timeout = std::stol(argv[i + 1]);
        }
        if (arg == "--user-agent" && i + 1 < argc) {
            config.user_agent = argv[i + 1];
        }
        if (arg == "--output-dir" && i + 1 < argc) {
            config.output_dir = argv[i + 1];
        }
        if (arg == "--headless") {
            config.enable_headless_rendering = true;
        }
        if (arg == "--chrome-path" && i + 1 < argc) {
            config.chrome_path = argv[i + 1];
        }
        if (arg == "--chrome-timeout" && i + 1 < argc) {
            config.chrome_timeout_seconds = std::stoi(argv[i + 1]);
        }
        if (arg == "--clickhouse-enabled") {
            config.clickhouse_enabled = true;
        }
        if (arg == "--clickhouse-endpoint" && i + 1 < argc) {
            config.clickhouse_endpoint = argv[i + 1];
        }
        if (arg == "--clickhouse-db" && i + 1 < argc) {
            config.clickhouse_database = argv[i + 1];
        }
        if (arg == "--clickhouse-metrics-table" && i + 1 < argc) {
            config.clickhouse_metrics_table = argv[i + 1];
        }
        if (arg == "--clickhouse-link-table" && i + 1 < argc) {
            config.clickhouse_link_graph_table = argv[i + 1];
        }
        if (arg == "--clickhouse-user" && i + 1 < argc) {
            config.clickhouse_user = argv[i + 1];
        }
        if (arg == "--clickhouse-password" && i + 1 < argc) {
            config.clickhouse_password = argv[i + 1];
        }
        if (arg == "--clickhouse-timeout" && i + 1 < argc) {
            config.clickhouse_timeout_seconds = std::stoi(argv[i + 1]);
        }
        if (arg == "--api-enabled") {
            config.api_enabled = true;
        }
        if (arg == "--api-bind" && i + 1 < argc) {
            config.api_bind_address = argv[i + 1];
        }
        if (arg == "--api-port" && i + 1 < argc) {
            config.api_port = std::stoi(argv[i + 1]);
        }
    }
    
    return config;
}

CrawlerConfig ConfigLoader::get_default() {
    CrawlerConfig config;
    
    // Add some default headers
    config.headers["Accept-Language"] = "en-US,en;q=0.9";
    config.headers["Accept-Encoding"] = "gzip, deflate";
    config.headers["Cache-Control"] = "no-cache";
    
    return config;
}

void ConfigLoader::save(const std::string& filepath, const CrawlerConfig& config) {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filepath);
        }

        file << "{\n";
        file << "  \"crawler\": {\n";
        file << "    \"timeout\": " << config.timeout << ",\n";
        file << "    \"max_retries\": " << config.max_retries << ",\n";
        file << "    \"user_agent\": \"" << config.user_agent << "\",\n";
        file << "    \"follow_redirects\": " << (config.follow_redirects ? "true" : "false") << ",\n";
        file << "    \"respect_robots_txt\": " << (config.respect_robots_txt ? "true" : "false") << ",\n";
        file << "    \"respect_meta_tags\": " << (config.respect_meta_tags ? "true" : "false") << "\n";
        file << "  },\n";

        file << "  \"output\": {\n";
        file << "    \"format\": \"" << config.output_format << "\",\n";
        file << "    \"output_dir\": \"" << config.output_dir << "\",\n";
        file << "    \"batch_size\": " << config.batch_size << "\n";
        file << "  },\n";

        file << "  \"urls\": [\n";
        for (size_t i = 0; i < config.urls.size(); i++) {
            file << "    \"" << config.urls[i] << "\"";
            if (i < config.urls.size() - 1) {
                file << ",\n";
            } else {
                file << "\n";
            }
        }
        file << "  ],\n";

        file << "  \"headers\": {\n";
        size_t header_count = 0;
        for (const auto& [key, value] : config.headers) {
            file << "    \"" << key << "\": \"" << value << "\"";
            header_count++;
            if (header_count < config.headers.size()) {
                file << ",\n";
            } else {
                file << "\n";
            }
        }
        file << "  },\n";
        file << "  \"headless\": {\n";
        file << "    \"enabled\": " << (config.enable_headless_rendering ? "true" : "false") << ",\n";
        file << "    \"chrome_path\": \"" << config.chrome_path << "\",\n";
        file << "    \"timeout_seconds\": " << config.chrome_timeout_seconds << "\n";
        file << "  },\n";
        file << "  \"clickhouse\": {\n";
        file << "    \"enabled\": " << (config.clickhouse_enabled ? "true" : "false") << ",\n";
        file << "    \"endpoint\": \"" << config.clickhouse_endpoint << "\",\n";
        file << "    \"database\": \"" << config.clickhouse_database << "\",\n";
        file << "    \"metrics_table\": \"" << config.clickhouse_metrics_table << "\",\n";
        file << "    \"link_graph_table\": \"" << config.clickhouse_link_graph_table << "\",\n";
        file << "    \"user\": \"" << config.clickhouse_user << "\",\n";
        file << "    \"password\": \"" << config.clickhouse_password << "\",\n";
        file << "    \"timeout_seconds\": " << config.clickhouse_timeout_seconds << "\n";
        file << "  },\n";
        file << "  \"api\": {\n";
        file << "    \"enabled\": " << (config.api_enabled ? "true" : "false") << ",\n";
        file << "    \"bind_address\": \"" << config.api_bind_address << "\",\n";
        file << "    \"port\": " << config.api_port << "\n";
        file << "  }\n";
        file << "}\n";

        file.close();

        std::ostringstream msg;
        msg << "Saved configuration to " << filepath;
        log_info(msg.str());

    } catch (const std::exception& e) {
        std::ostringstream error_msg;
        error_msg << "Failed to save configuration: " << e.what();
        log_error(error_msg.str());
        throw;
    }
}

std::string ConfigLoader::read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

CrawlerConfig ConfigLoader::parse_json(const std::string& json_str) {
    CrawlerConfig config = get_default();

    try {
        // Simple JSON parsing (without external library)
        
        // Extract timeout
        size_t timeout_pos = json_str.find("\"timeout\"");
        if (timeout_pos != std::string::npos) {
            size_t colon_pos = json_str.find(":", timeout_pos);
            size_t comma_pos = json_str.find(",", colon_pos);
            std::string timeout_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
            timeout_str.erase(0, timeout_str.find_first_not_of(" \t\n\r"));
            config.timeout = std::stol(timeout_str);
        }

        // Extract max_retries
        size_t retries_pos = json_str.find("\"max_retries\"");
        if (retries_pos != std::string::npos) {
            size_t colon_pos = json_str.find(":", retries_pos);
            size_t comma_pos = json_str.find(",", colon_pos);
            std::string retries_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
            retries_str.erase(0, retries_str.find_first_not_of(" \t\n\r"));
            config.max_retries = std::stoi(retries_str);
        }

        // Extract user_agent
        size_t ua_pos = json_str.find("\"user_agent\"");
        if (ua_pos != std::string::npos) {
            size_t colon_pos = json_str.find(":", ua_pos);
            size_t quote1_pos = json_str.find("\"", colon_pos);
            size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
            config.user_agent = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
        }

        // Extract output_format
        size_t fmt_pos = json_str.find("\"format\"");
        if (fmt_pos != std::string::npos) {
            size_t colon_pos = json_str.find(":", fmt_pos);
            size_t quote1_pos = json_str.find("\"", colon_pos);
            size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
            config.output_format = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
        }

        // Extract output_dir
        size_t dir_pos = json_str.find("\"output_dir\"");
        if (dir_pos != std::string::npos) {
            size_t colon_pos = json_str.find(":", dir_pos);
            size_t quote1_pos = json_str.find("\"", colon_pos);
            size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
            config.output_dir = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
        }

        // Extract headless settings
        size_t headless_pos = json_str.find("\"headless\"");
        if (headless_pos != std::string::npos) {
            size_t enabled_pos = json_str.find("\"enabled\"", headless_pos);
            if (enabled_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", enabled_pos);
                size_t comma_pos = json_str.find(",", colon_pos);
                std::string enabled_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                enabled_str.erase(0, enabled_str.find_first_not_of(" \t\n\r"));
                config.enable_headless_rendering = enabled_str.find("true") != std::string::npos;
            }

            size_t chrome_path_pos = json_str.find("\"chrome_path\"", headless_pos);
            if (chrome_path_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", chrome_path_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.chrome_path = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t timeout_pos = json_str.find("\"timeout_seconds\"", headless_pos);
            if (timeout_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", timeout_pos);
                size_t comma_pos = json_str.find(",", colon_pos);
                std::string timeout_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                timeout_str.erase(0, timeout_str.find_first_not_of(" \t\n\r"));
                config.chrome_timeout_seconds = std::stoi(timeout_str);
            }
        }

        // Extract ClickHouse settings
        size_t clickhouse_pos = json_str.find("\"clickhouse\"");
        if (clickhouse_pos != std::string::npos) {
            size_t enabled_pos = json_str.find("\"enabled\"", clickhouse_pos);
            if (enabled_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", enabled_pos);
                size_t comma_pos = json_str.find(",", colon_pos);
                std::string enabled_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                enabled_str.erase(0, enabled_str.find_first_not_of(" \t\n\r"));
                config.clickhouse_enabled = enabled_str.find("true") != std::string::npos;
            }

            size_t endpoint_pos = json_str.find("\"endpoint\"", clickhouse_pos);
            if (endpoint_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", endpoint_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.clickhouse_endpoint = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t database_pos = json_str.find("\"database\"", clickhouse_pos);
            if (database_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", database_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.clickhouse_database = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t metrics_pos = json_str.find("\"metrics_table\"", clickhouse_pos);
            if (metrics_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", metrics_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.clickhouse_metrics_table = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t link_table_pos = json_str.find("\"link_graph_table\"", clickhouse_pos);
            if (link_table_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", link_table_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.clickhouse_link_graph_table = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t user_pos = json_str.find("\"user\"", clickhouse_pos);
            if (user_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", user_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.clickhouse_user = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t password_pos = json_str.find("\"password\"", clickhouse_pos);
            if (password_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", password_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.clickhouse_password = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t timeout_pos = json_str.find("\"timeout_seconds\"", clickhouse_pos);
            if (timeout_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", timeout_pos);
                size_t comma_pos = json_str.find(",", colon_pos);
                std::string timeout_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                timeout_str.erase(0, timeout_str.find_first_not_of(" \t\n\r"));
                config.clickhouse_timeout_seconds = std::stoi(timeout_str);
            }
        }

        // Extract API settings
        size_t api_pos = json_str.find("\"api\"");
        if (api_pos != std::string::npos) {
            size_t enabled_pos = json_str.find("\"enabled\"", api_pos);
            if (enabled_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", enabled_pos);
                size_t comma_pos = json_str.find(",", colon_pos);
                std::string enabled_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                enabled_str.erase(0, enabled_str.find_first_not_of(" \t\n\r"));
                config.api_enabled = enabled_str.find("true") != std::string::npos;
            }

            size_t bind_pos = json_str.find("\"bind_address\"", api_pos);
            if (bind_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", bind_pos);
                size_t quote1_pos = json_str.find("\"", colon_pos);
                size_t quote2_pos = json_str.find("\"", quote1_pos + 1);
                config.api_bind_address = json_str.substr(quote1_pos + 1, quote2_pos - quote1_pos - 1);
            }

            size_t port_pos = json_str.find("\"port\"", api_pos);
            if (port_pos != std::string::npos) {
                size_t colon_pos = json_str.find(":", port_pos);
                size_t comma_pos = json_str.find(",", colon_pos);
                std::string port_str = json_str.substr(colon_pos + 1, comma_pos - colon_pos - 1);
                port_str.erase(0, port_str.find_first_not_of(" \t\n\r"));
                config.api_port = std::stoi(port_str);
            }
        }

        // Extract URLs
        config.urls.clear();
        size_t urls_pos = json_str.find("\"urls\"");
        if (urls_pos != std::string::npos) {
            size_t bracket_pos = json_str.find("[", urls_pos);
            size_t end_bracket_pos = json_str.find("]", bracket_pos);
            std::string urls_section = json_str.substr(bracket_pos + 1, end_bracket_pos - bracket_pos - 1);

            size_t start = 0;
            while ((start = urls_section.find("\"", start)) != std::string::npos) {
                size_t end = urls_section.find("\"", start + 1);
                if (end != std::string::npos) {
                    std::string url = urls_section.substr(start + 1, end - start - 1);
                    config.urls.push_back(url);
                    start = end + 1;
                } else {
                    break;
                }
            }
        }

        // Extract headers
        config.headers.clear();
        size_t headers_pos = json_str.find("\"headers\"");
        if (headers_pos != std::string::npos) {
            size_t brace_pos = json_str.find("{", headers_pos);
            size_t end_brace_pos = json_str.find("}", brace_pos);
            std::string headers_section = json_str.substr(brace_pos + 1, end_brace_pos - brace_pos - 1);

            size_t start = 0;
            while ((start = headers_section.find("\"", start)) != std::string::npos) {
                size_t colon = headers_section.find("\"", start + 1);
                if (colon == std::string::npos) break;

                std::string key = headers_section.substr(start + 1, colon - start - 1);

                size_t val_start = headers_section.find("\"", colon + 1);
                if (val_start == std::string::npos) break;

                size_t val_end = headers_section.find("\"", val_start + 1);
                if (val_end == std::string::npos) break;

                std::string value = headers_section.substr(val_start + 1, val_end - val_start - 1);
                config.headers[key] = value;

                start = val_end + 1;
            }
        }

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse JSON: ") + e.what());
    }

    return config;
}
