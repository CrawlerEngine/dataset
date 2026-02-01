# Краткое руководство - Dataset Crawler

## ⚡ Самые быстрые примеры

### Одиночный URL
```bash
cd build
./crawler --url "https://example.com"
```

### Несколько URL
```bash
./crawler --urls "https://site1.com,https://site2.com,https://site3.com"
```

### С кастомным User-Agent
```bash
./crawler --url "https://mysite.com" --user-agent "MyBot/1.0"
```

### С конфиг файлом
```bash
./crawler --config ../config.json
```

### С увеличенным timeout
```bash
./crawler --url "https://mysite.com" --timeout 60
```

## ⚠️ LinkedIn - СТРОГО ЗАПРЕЩЕНО!

```bash
# ❌ НЕ ДЕЛАЙ ЭТО!
./crawler --url "https://linkedin.com"
```

LinkedIn **явно запрещает** автоматизированный краулинг в своих Terms of Service. Это может привести к:
- 🚫 Блокировке аккаунта
- ⚖️ Судебным искам
- 💰 Гражданским штрафам

**Законные способы:**
1. LinkedIn API - официальный способ
2. LinkedIn Data Scrape - платный сервис
3. Ручной сбор данных
4. Публичные датасеты (LinkedIn Research, Kaggle)

## 📚 Реальные примеры

### Wikipedia (✅ разрешено)
```bash
./crawler --urls "https://en.wikipedia.org/wiki/Machine_learning,https://en.wikipedia.org/wiki/Artificial_intelligence"
```

### GitHub (✅ разрешено)
make build
```

Или пошагово:
```bash
mkdir -p build
cd build
cmake ..
make
cd ..
```

### 3. Запуск краулера

```bash
make run
```

Или напрямую:
```bash
./build/crawler
```

## Примеры использования

### Пример 1: Простой краулинг

```cpp
#include "crawler.h"
#include "dataset_writer.h"

int main() {
    WebCrawler crawler;
    DataRecord record = crawler.fetch("https://example.com");
    
    ParquetDatasetWriter writer;
    std::vector<DataRecord> records = {record};
    writer.write_records("data.parquet", records);
    
    return 0;
}
```

### Пример 2: Многопоточный краулинг

```cpp
#include "advanced_crawler.h"

int main() {
    AdvancedCrawler crawler(8);  // 8 потоков
    
    std::vector<std::string> urls = {
        "https://example.com/1",
        "https://example.com/2",
        "https://example.com/3"
    };
    
    auto records = crawler.crawl_parallel(urls);
    
    ParquetDatasetWriter writer;
    writer.write_records("dataset.parquet", records);
    
    return 0;
}
```

### Пример 3: Краулинг из файла

```cpp
#include "advanced_crawler.h"

int main() {
    AdvancedCrawler crawler(4);
    auto records = crawler.crawl_from_file("urls.txt");
    
    // Сохранить результаты
    ParquetDatasetWriter writer;
    writer.write_records("output.parquet", records);
    
    return 0;
}
```

## Работа с Parquet файлами

### Python - Чтение данных

```python
import pandas as pd

# Прочитать Parquet файл
df = pd.read_parquet('dataset.parquet')

# Просмотреть информацию
print(df.info())
print(df.head())

# Статистика
print(df['status_code'].value_counts())
```

### Python - Использование утилит

```bash
# Просмотреть информацию о датасете
python3 scripts/parquet_utils.py info dataset.parquet

# Конвертировать в CSV
python3 scripts/parquet_utils.py to-csv dataset.parquet dataset.csv

# Конвертировать в JSON
python3 scripts/parquet_utils.py to-json dataset.parquet dataset.json

# Слить несколько файлов
python3 scripts/parquet_utils.py merge part1.parquet part2.parquet -o merged.parquet

# Фильтровать по статус коду
python3 scripts/parquet_utils.py filter dataset.parquet 200 -o success.parquet

# Взять образец данных
python3 scripts/parquet_utils.py sample dataset.parquet 100 -o sample.parquet
```

## Docker использование

### Сборка образа

```bash
make docker-build
```

### Запуск в контейнере

```bash
make docker-run
```

### Полный Docker Compose

```bash
docker-compose up
```

## Структура проекта

```
dataset/
├── CMakeLists.txt              # Конфигурация сборки
├── Makefile                    # Удобные команды сборки
├── Dockerfile                  # Docker образ
├── docker-compose.yml          # Docker Compose конфиг
├── requirements.txt            # Python зависимости
├── config.json                 # Конфиг краулера
├── urls.txt                    # Пример URL-ов
│
├── include/
│   ├── crawler.h               # Базовый краулер
│   ├── dataset_writer.h        # Запись в Parquet/CSV
│   └── advanced_crawler.h      # Многопоточный краулер
│
├── src/
│   ├── main.cpp                # Главная программа
│   ├── crawler.cpp             # Реализация краулера
│   ├── dataset_writer.cpp      # Реализация Parquet
│   └── advanced_crawler.cpp    # Многопоточность
│
├── examples/
│   └── examples.cpp            # Примеры кода
│
├── scripts/
│   └── parquet_utils.py        # Python утилиты
│
└── README.md                   # Полная документация
```

## Основные классы

### WebCrawler

```cpp
class WebCrawler {
    // Получить одну страницу
    DataRecord fetch(const std::string& url);
    
    // Краулить несколько страниц
    std::vector<DataRecord> crawl_urls(const std::vector<std::string>& urls);
    
    // Установить таймаут
    void set_timeout(long seconds);
    
    // Добавить заголовок
    void add_header(const std::string& key, const std::string& value);
};
```

### AdvancedCrawler

```cpp
class AdvancedCrawler {
    // Краулить с многопоточностью
    std::vector<DataRecord> crawl_parallel(const std::vector<std::string>& urls);
    
    // Краулить из файла
    std::vector<DataRecord> crawl_from_file(const std::string& filename);
    
    // Получить статистику
    Stats get_stats() const;
};
```

### ParquetDatasetWriter

```cpp
class ParquetDatasetWriter {
    // Записать в Parquet
    void write_records(const std::string& filepath, 
                      const std::vector<DataRecord>& records);
    
    // Добавить к существующему файлу
    void append_records(const std::string& filepath,
                       const std::vector<DataRecord>& records);
    
    // Записать в CSV
    void write_csv(const std::string& filepath,
                  const std::vector<DataRecord>& records);
};
```

## Структура данных

```cpp
struct DataRecord {
    std::string url;        // URL источника
    std::string title;      // Заголовок страницы
    std::string content;    // HTML контент
    std::string timestamp;  // Время сбора
    int status_code;        // HTTP статус
};
```

## Разрешение проблем

### Ошибка: libcurl не найдена
```bash
sudo apt-get install libcurl4-openssl-dev
```

### Ошибка: Parquet не найдена
```bash
sudo apt-get install libparquet-dev libparquet0
```

### Ошибка: CMake не найдена
```bash
sudo apt-get install cmake
```

### Проблемы с сетью
- Проверьте интернет соединение
- Увеличьте таймаут: `crawler.set_timeout(60)`
- Используйте прокси если необходимо

## Оптимизация производительности

### 1. Используйте многопоточность
```cpp
AdvancedCrawler crawler(8);  // 8 потоков
auto records = crawler.crawl_parallel(urls);
```

### 2. Батч-запись
```cpp
std::vector<DataRecord> batch;
for (const auto& record : records) {
    batch.push_back(record);
    if (batch.size() >= 1000) {
        writer.write_records("part.parquet", batch);
        batch.clear();
    }
}
```

### 3. Сжатие данных
- Parquet автоматически компрессирует данные
- Снижает размер файла в 5-10 раз

### 4. Параллельный доступ к файлам
```bash
python3 scripts/parquet_utils.py merge part*.parquet -o final.parquet
```

## Примеры команд

```bash
# Сборка и запуск
make clean build run

# Установить как системную команду
make install

# Использовать Docker
docker build -t crawler .
docker run -v $(pwd):/app crawler

# Анализировать данные
python3 -c "import pandas; df=pandas.read_parquet('dataset.parquet'); print(df.info())"
```

## Дополнительные ресурсы

- [Apache Arrow documentation](https://arrow.apache.org/)
- [Parquet format](https://parquet.apache.org/)
- [libcurl documentation](https://curl.se/libcurl/)
- [CMake documentation](https://cmake.org/documentation/)

## Лицензия

MIT License - Свободное использование в личных и коммерческих целях.
