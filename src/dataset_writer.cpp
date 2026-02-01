#include "dataset_writer.h"
#include "logger.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

ParquetDatasetWriter::ParquetDatasetWriter() {
}

ParquetDatasetWriter::~ParquetDatasetWriter() {
}

void ParquetDatasetWriter::write_records(const std::string& filepath, 
                                         const std::vector<DataRecord>& records) {
    if (filepath.find(".json") != std::string::npos) {
        write_json_internal(filepath, records);
    } else {
        write_json_internal(filepath + ".json", records);
    }
}

void ParquetDatasetWriter::append_records(const std::string& filepath,
                                          const std::vector<DataRecord>& records) {
    // For append, we'll write to a new file
    write_json_internal(filepath, records);
}

void ParquetDatasetWriter::write_json_internal(const std::string& filepath,
                                               const std::vector<DataRecord>& records) {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open JSON file: " + filepath);
        }

        file << "[\n";
        
        for (size_t i = 0; i < records.size(); ++i) {
            const auto& record = records[i];
            
            file << "  {\n";
            file << "    \"url\": \"" << escape_json(record.url) << "\",\n";
            file << "    \"title\": \"" << escape_json(record.title) << "\",\n";
            file << "    \"content_length\": " << record.content.size() << ",\n";
            file << "    \"timestamp\": \"" << record.timestamp << "\",\n";
            file << "    \"status_code\": " << record.status_code << "\n";
            file << "  }";
            
            if (i < records.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "]\n";
        file.close();
        
        std::ostringstream msg;
        msg << "Successfully wrote " << records.size() << " records to " << filepath;
        log_info(msg.str());

    } catch (const std::exception& e) {
        std::ostringstream error_msg;
        error_msg << "Error writing JSON file: " << e.what();
        log_error(error_msg.str());
        throw;
    }
}

void ParquetDatasetWriter::write_csv(const std::string& filepath,
                                     const std::vector<DataRecord>& records) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + filepath);
    }

    // Write header
    file << "url,title,content_length,timestamp,status_code\n";

    // Write records
    for (const auto& record : records) {
        auto escape_csv = [](const std::string& value) -> std::string {
            std::string escaped = "\"";
            for (char c : value) {
                if (c == '"') {
                    escaped += "\"\"";
                } else if (c == '\n' || c == '\r') {
                    escaped += " ";
                } else {
                    escaped += c;
                }
            }
            escaped += "\"";
            return escaped;
        };

        file << escape_csv(record.url) << ","
             << escape_csv(record.title) << ","
             << record.content.size() << ","
             << escape_csv(record.timestamp) << ","
             << record.status_code << "\n";
    }

    file.close();
    
    std::ostringstream msg;
    msg << "Successfully wrote " << records.size() << " records to " << filepath;
    log_info(msg.str());
}

std::string ParquetDatasetWriter::escape_json(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (c < 32) {
                    escaped += "\\u" + std::to_string(static_cast<int>(c));
                } else {
                    escaped += c;
                }
        }
    }
    return escaped;
}
