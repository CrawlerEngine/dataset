#ifndef DATASET_WRITER_H
#define DATASET_WRITER_H

#include "crawler.h"
#include <vector>
#include <string>
#include <memory>

class ParquetDatasetWriter {
public:
    ParquetDatasetWriter();
    ~ParquetDatasetWriter();

    /**
     * Write records to JSON file (with .json extension)
     */
    void write_records(const std::string& filepath, const std::vector<DataRecord>& records);

    /**
     * Append records to existing file
     */
    void append_records(const std::string& filepath, const std::vector<DataRecord>& records);

    /**
     * Write records to CSV file
     */
    void write_csv(const std::string& filepath, const std::vector<DataRecord>& records);

private:
    void write_json_internal(const std::string& filepath, 
                            const std::vector<DataRecord>& records);
    
    std::string escape_json(const std::string& value);
};

#endif // DATASET_WRITER_H
