# Dataset Crawler - C++ Web Crawler with JSON/CSV Support

Краулер для сбора датасетов для обучения ИИ с экспортом в JSON и CSV.

## Возможности

- ✓ Многопоточный веб-краулинг
- ✓ Сохранение в JSON (легко читается)
- ✓ Экспорт в CSV (для Excel/Pandas)
- ✓ **Поддержка robots.txt** (этичный краулинг)
- ✓ **Проверка meta-тегов noindex/nofollow**
- ✓ **Парсинг Sitemap** (автоматическое обнаружение URLs в robots.txt и sitemap.xml)
- ✓ **Лимит размера файлов** (по умолчанию 100 МБ, настраивается)
- ✓ **Детальная статистика** (JSON формат с метриками запросов)
- ✓ **ISO 8601 логирование** с цветовой кодировкой (INFO/WARN/ERROR)
- ✓ HTTP кэширование
- ✓ Обработка ошибок сети
- ✓ Настраиваемые заголовки и таймауты
- ✓ Конфигурация через JSON или command-line аргументы

## Структура проекта

```
.
├── WORKSPACE                   # Конфигурация Bazel монорепозитория
├── BUILD                       # Цели сборки Bazel
├── .bazelrc                    # Параметры Bazel
├── .bazelignore               # Исключаемые файлы для Bazel
├── bazel/                      # Конфиги внешних зависимостей
├── docs/                       # Документация
│   ├── v3.0/                   # v3.0 docs (RocksDB, Gumbo, Markdown)
│   ├── v2.7/                   # v2.7 docs (Wildcard path matching)
│   ├── api/                    # API документация
│   └── guides/                 # Гайды
├── include/
│   ├── crawler.h               # Интерфейс веб-краулера
│   └── dataset_writer.h        # Интерфейс записи данных
├── src/
│   ├── main.cpp                # Точка входа программы
│   ├── crawler.cpp             # Реализация краулера
│   └── dataset_writer.cpp      # Реализация записи Parquet/CSV
└── install_dependencies.sh     # Скрипт установки зависимостей
```

## Установка

### 1. Установка зависимостей

```bash
chmod +x install_dependencies.sh
./install_dependencies.sh
```

Или вручную:

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential
sudo apt-get install -y libcurl4-openssl-dev
sudo apt-get install -y libparquet-dev libparquet0
```

### 2. Сборка проекта

```bash
mkdir build && cd build
cmake ..
make
```

### 3. Запуск

```bash
./crawler
```

## Использование

### Базовый пример

```cpp
#include "crawler.h"
#include "dataset_writer.h"

int main() {
    // Создать краулер
    WebCrawler crawler;
    crawler.set_timeout(30);
    
    // Включить проверки (по умолчанию включены)
    crawler.set_respect_robots_txt(true);
    crawler.set_respect_meta_tags(true);
    
    // Краулить несколько страниц
    std::vector<std::string> urls = {
        "https://example.com/page1",
        "https://example.com/page2",
        "https://example.com/page3"
    };
    auto records = crawler.crawl_urls(urls);
    
    // Получить статистику
    std::cout << "Собрано: " << records.size() << std::endl;
    std::cout << "Пропущено robots.txt: " << crawler.get_blocked_by_robots_count() << std::endl;
    std::cout << "Пропущено noindex: " << crawler.get_blocked_by_noindex_count() << std::endl;
    
    // Сохранить в JSON
    ParquetDatasetWriter writer;
    writer.write_records("dataset.json", records);
    
    return 0;
}
```

## API Краулера

### WebCrawler

```cpp
class WebCrawler {
public:
    // Создать краулер с кастомным User-Agent
    WebCrawler(const std::string& user_agent = "DatasetCrawler/1.0");
    
    // Получить одну страницу
    DataRecord fetch(const std::string& url);
    
    // Краулить несколько страниц
    std::vector<DataRecord> crawl_urls(const std::vector<std::string>& urls);
    
    // Установить таймаут (в секундах)
    void set_timeout(long timeout_seconds);
    
    // Добавить кастомный заголовок
    void add_header(const std::string& key, const std::string& value);
    
    // Включить/отключить проверки этичного краулинга
    void set_respect_robots_txt(bool respect);      // По умолчанию true
    void set_respect_meta_tags(bool respect);       // По умолчанию true
    
    // Получить статистику
    int get_blocked_by_robots_count() const;
    int get_blocked_by_noindex_count() const;
};
```

### ParquetDatasetWriter

```cpp
class ParquetDatasetWriter {
public:
    // Записать записи в Parquet файл
    void write_records(const std::string& filepath, 
                      const std::vector<DataRecord>& records);
    
    // Добавить записи к существующему файлу
    void append_records(const std::string& filepath,
                       const std::vector<DataRecord>& records);
    
    // Написать в CSV формате
    void write_csv(const std::string& filepath,
                  const std::vector<DataRecord>& records);
};
```

## Структура данных

Каждая запись содержит:

```cpp
struct DataRecord {
    std::string url;           // URL источника
    std::string title;         // Заголовок страницы
    std::string content;       // Основной контент (HTML)
    std::string timestamp;     // Время сбора данных
    int status_code;          // HTTP статус код
    bool was_allowed;         // Разрешена ли страница (robots.txt, meta tags)
};
```

## Формат данных

Файл JSON имеет следующую структуру:

```json
[
  {
    "url": "https://example.com",
    "title": "Page Title",
    "content_length": 12345,
    "timestamp": "2026-01-25 10:30:45",
    "status_code": 200
  },
  ...
]
```

## Примеры использования

### Пример 1: Простой краулинг

```bash
./crawler
```

Будет загружена парочка тестовых страниц и сохранены в `dataset.json`.

### Пример 2: Читать JSON в Python

```python
import json
import pandas as pd

# Прочитать JSON файл
with open('dataset.json', 'r') as f:
    data = json.load(f)

# Или использовать pandas
df = pd.read_json('dataset.json')

# Просмотреть данные
print(df.head())
print(df.info())

# Экспортировать в CSV
df.to_csv('dataset.csv', index=False)
```

### Пример 3: Анализ в Python

```python
import json

with open('dataset.json', 'r') as f:
    data = json.load(f)

# Статистика по статус кодам
status_codes = {}
for item in data:
    code = item['status_code']
    status_codes[code] = status_codes.get(code, 0) + 1

print("HTTP Status Codes:")
for code, count in sorted(status_codes.items()):
    print(f"  {code}: {count}")

# Размер контента
print("\nContent Statistics:")
lengths = [item['content_length'] for item in data]
print(f"  Average: {sum(lengths) / len(lengths):.0f} bytes")
print(f"  Min: {min(lengths)} bytes")
print(f"  Max: {max(lengths)} bytes")
```

## Оптимизация для больших датасетов

### Параллельный краулинг (расширенная версия)

```cpp
// Модифицируйте crawler для многопоточности
std::vector<std::thread> threads;
for (const auto& url : urls) {
    threads.emplace_back([&, url]() {
        records.push_back(crawler.fetch(url));
    });
}
for (auto& t : threads) t.join();
```

### Батч-запись

```cpp
ParquetDatasetWriter writer;
std::vector<DataRecord> batch;

for (const auto& record : all_records) {
    batch.push_back(record);
    
    // Записать батч каждые 1000 записей
    if (batch.size() >= 1000) {
        writer.write_records("dataset_part.parquet", batch);
        batch.clear();
    }
}

// Последняя партия
if (!batch.empty()) {
    writer.write_records("dataset_final.parquet", batch);
}
```

## Производительность

- Скорость краулинга: зависит от сетевых задержек (обычно 1-5 запросов в секунду)
- Размер файла: JSON примерно такой же как CSV (текстовый формат)
- Скорость чтения: Очень быстро в Python с json/pandas

## Новые возможности (v2.0)

### 1. Парсинг Sitemap

Краулер автоматически обнаруживает и парсит Sitemap директивы из robots.txt:

```bash
# Будут автоматически найдены все Sitemap: URLs
./crawler --url https://yourdomain.com --max-depth 2
```

Методы API:
```cpp
// Получить Sitemap URLs из robots.txt домена
std::vector<std::string> sitemaps = crawler.get_sitemaps_from_robots("yourdomain.com");

// Загрузить URLs из sitemap.xml
std::vector<std::string> urls = crawler.fetch_sitemap_urls("https://yourdomain.com/sitemap.xml");

// Сколько найдено sitemaps
int count = crawler.get_sitemaps_found_count();
```

### 2. Лимит размера файлов

Пропускайте слишком большие файлы чтобы экономить трафик:

```cpp
crawler.set_max_file_size(100);  // 100 МБ максимум (по умолчанию)
```

Логирование:
```
2026-01-25T08:49:02.932Z WARN  Skipped https://example.com/large.pdf - file size 150MB exceeds limit
```

Статистика:
```cpp
int skipped = crawler.get_skipped_by_size_count();  // Количество пропущенных
```

### 3. Детальная статистика

Получайте подробные метрики краулинга:

```cpp
CrawlerStats stats = crawler.get_statistics();
// stats.total_requests
// stats.successful_requests
// stats.failed_requests
// stats.blocked_by_robots
// stats.skipped_by_size
// stats.sitemaps_found
// stats.total_bytes_downloaded
// stats.avg_request_duration_ms
// stats.requests_per_minute
```

Пример JSON логирования статистики:
```json
{
  "requestAvgFailedDurationMillis": null,
  "requestAvgFinishedDurationMillis": 23,
  "requestsFinishedPerMinute": 2553,
  "requestsFailedPerMinute": 0,
  "requestTotalDurationMillis": 47,
  "requestsTotal": 1,
  "crawlerRuntimeMillis": 47,
  "retryHistogram": [1]
}
```

### 4. Профессиональное логирование

Цветное ISO 8601 логирование с разными уровнями:

```
2026-01-25T08:49:02.932Z INFO   Loaded configuration from config.json
2026-01-25T08:49:02.933Z INFO   Starting the crawler
2026-01-25T08:49:02.984Z WARN   No text parsed from https://example.com
2026-01-25T08:49:02.985Z ERROR  Failed to fetch sitemap
```

## Сборка и тестирование

### С использованием Bazel (рекомендуется)

**Требования:** Bazel 6.0+, C++17 компилятор

```bash
# Собрать все целевые объекты
bazel build //...

# Запустить все тесты
bazel test //...

# Собрать только main binary
bazel build //:crawler

# Запустить конкретный тест
bazel test //:rocksdb_test
```

**Конфигурация:** Параметры находятся в `.bazelrc`:
- Стандарт C++17
- Оптимизация уровня -O2
- Параллельная сборка (-j auto)

### Структура тестов

Проект включает 30 gTest тестов:

| Модуль | Тесты | Покрытие |
|--------|-------|---------|
| RocksDB | 7 | Инициализация БД, очередь, кэширование, персистентность |
| TextExtractor | 7 | Парсинг Markdown, code detection, форматирование |
| RobotsUAPriority | 4 | Приоритизация user-agents, wildcard fallback |
| RobotsIntegration | 3 | Парсинг robots.txt, multiple rules |
| RobotsWildcard | 9 | Wildcard patterns, longest match, case sensitivity |

**Запуск тестов с детальным выводом:**
```bash
bazel test //... --test_output=all
```

### 5. Автоматическое следование по ссылкам

Краулер автоматически извлекает и обходит все ссылки со страниц:

```cpp
// Краулер автоматически:
// 1. Извлекает все <a href="..."> ссылки со страницы
// 2. Обрабатывает относительные URL (../../page, /page, page)
// 3. Нормализует URL (убирает фрагменты #, приводит в единообразный вид)
// 4. Проверяет Canonical тег <link rel="canonical">
// 5. Избегает дублей (не посещает одну страницу дважды)
// 6. Логирует найденные ссылки: "Enqueued 8 new links on https://example.com/"

auto records = crawler.crawl_urls({"https://example.com"});
```

**Логирование извлечения ссылок:**
```
2026-01-25T09:15:22.530Z INFO   http://example.com/ [200]
2026-01-25T09:15:22.533Z INFO   Enqueued 5 new links on http://example.com/
2026-01-25T09:15:22.535Z INFO   http://example.com/page1 [200]
2026-01-25T09:15:22.543Z INFO   http://example.com/page2 [200]
```

**API для работы со ссылками:**
```cpp
// Извлечь все ссылки из HTML
std::vector<std::string> links = crawler.extract_links_from_html(html, base_url);

// Нормализовать URL (убирает фрагменты, приводит в единообразный вид)
std::string normalized = crawler.normalize_url("https://Example.COM/page#section");
// Результат: "https://example.com/page"

// Разрешить относительный URL в абсолютный
std::string absolute = crawler.resolve_relative_url(
    "https://example.com/dir/page.html",
    "../other/link"
);
// Результат: "https://example.com/other/link"

// Найти Canonical URL (если присутствует на странице)
std::string canonical = crawler.extract_canonical_url(html, base_url);
```

**Обработка разных типов URL:**
- ✅ Абсолютные: `https://example.com/page`
- ✅ Абсолютные пути: `/page`
- ✅ Относительные: `page`, `../page`
- ✅ Protocol-relative: `//example.com/page`
- ✅ Фрагменты: автоматически удаляются
- ✅ JavaScript/mailto: пропускаются
- ✅ Canonical: извлекаются и добавляются в очередь

## Лицензия

MIT License

## Этичный краулинг

**Важно:** Этот краулер по умолчанию **соблюдает** веб-стандарты:

- ✅ Проверяет `robots.txt` перед загрузкой страниц
- ✅ Проверяет мета-теги `noindex` и `nofollow`
- ✅ Логирует пропущенные страницы
- ✅ Кэширует robots.txt для оптимизации
- ✅ Пропускает большие файлы (> 100 МБ)
- ✅ Парсит Sitemap для правильного обхода
- ✅ Автоматически обходит только ссылки на том же домене
- ✅ Нормализует URL для избежания дублей

Подробнее см. [ETHICS.md](ETHICS.md)

## Автор

Dataset Crawler - C++ implementation for AI training datasets