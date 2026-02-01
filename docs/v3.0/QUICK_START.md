# v3.0 Quick Integration Guide

## 🚀 Быстрый Старт

### Требования
- CMake 3.20+
- GCC/Clang с C++17 поддержкой
- librocksdb-dev (RocksDB library)
- libgumbo-dev (HTML parser)

### Сборка
```bash
cd /workspaces/dataset/build
cmake ..
make
```

### Проверка
```bash
# Все компоненты
./test_rocksdb        # RocksDB - очередь и БД
./test_text_extractor # Gumbo - парсер HTML

# Старые компоненты (проверка отсутствия регрессии)
./test_robots_ua_priority
./test_robots_integration
./test_robots_wildcard

# Основное приложение
./crawler
```

---

## 📦 Компоненты v3.0

### 1. RocksDBManager
```cpp
#include "rocksdb_manager.h"

// Инициализация
RocksDBManager db_manager("/path/to/db");
db_manager.init();

// Работа с очередью
db_manager.enqueue_url("https://example.com");
while (db_manager.has_queued_urls()) {
    std::string url = db_manager.dequeue_url();
    // обработка URL
}

// Отслеживание посещённых
db_manager.mark_visited("https://example.com");
if (db_manager.is_visited("https://example.com")) {
    // уже посещали
}

// Кэширование HTML
db_manager.cache_html(url, html_content);
std::string cached = db_manager.get_cached_html(url);

// Статистика
std::cout << db_manager.get_stats();
```

### 2. TextExtractor
```cpp
#include "text_extractor.h"

TextExtractor extractor;

// Парсинг HTML
TextExtraction result = extractor.extract_from_html(html, base_url);

// Результаты
std::cout << result.title;           // Заголовок страницы
std::cout << result.text;            // Markdown текст
std::cout << result.plain_text;      // Чистый текст
for (auto& code : result.code_blocks) {
    std::cout << code;               // Код блоки с языками
}
for (auto& link : result.links) {
    std::cout << link;               // Извлеченные ссылки
}

// Настройка удаления элементов
extractor.set_remove_selectors("nav, footer, .ads, .sidebar");
```

---

## 🔄 Интеграция с WebCrawler (Будущее)

```cpp
class WebCrawler {
    // ...существующие методы...
    
    // Новое: инициализация БД
    void init_database(const std::string& db_path);
    
    // Новое: включение извлечения текста
    void enable_text_extraction(bool enable);
    
    // Новое: получение URL из очереди БД
    bool dequeue_next_url(std::string& url);
    
    // Новое: сохранение обработанного URL
    void mark_crawled(const std::string& url);
};

// Использование:
WebCrawler crawler;
crawler.init_database("/data/crawl.db");
crawler.enable_text_extraction(true);

std::vector<std::string> urls = {"https://example.com"};
auto records = crawler.crawl_urls(urls);

for (auto& record : records) {
    std::cout << "Title: " << record.title << "\n";
    std::cout << "Content:\n" << record.content << "\n";
    // Markdown уже отформатирован!
}
```

---

## 📊 Тестовые Результаты

### Текущие Тесты (6 исполняемых)

| Компонент | Статус | Тестов | Размер |
|-----------|--------|--------|---------|
| RocksDB Queue | ✅ | 6 | 127K |
| Text Extractor | ✅ | 4 | 637K |
| Robots UA Priority | ✅ | 21 | 993K |
| Robots Integration | ✅ | 16 | 1.0M |
| Robots Wildcard | ✅ | 37 | 1.1M |
| **Итого** | **✅ 84** | **84** | **4.8M** |

### Рун Тестов
```bash
cd /workspaces/dataset/build

# Быстрый прогон всех тестов
for test in test_*; do 
  echo "=== $test ===" && ./$test 2>&1 | tail -1
done

# Вывод:
# === test_rocksdb === ✓ All RocksDBManager tests passed!
# === test_text_extractor === ✓ All TextExtractor tests passed!
# === test_robots_ua_priority === ✓ All tests passed!
# === test_robots_integration === ✓ All integration tests passed!
# === test_robots_wildcard === ✓ All wildcard tests passed!
```

---

## 🔍 Примеры Использования

### Пример 1: Парсинг HTML
```cpp
#include "text_extractor.h"
#include <iostream>

int main() {
    std::string html = R"(
        <html>
            <head><title>My Page</title></head>
            <body>
                <nav>Menu</nav>
                <h1>Welcome</h1>
                <p>This is <strong>important</strong> text.</p>
                <pre><code>const x = 42;</code></pre>
                <footer>Footer</footer>
            </body>
        </html>
    )";
    
    TextExtractor extractor;
    auto result = extractor.extract_from_html(html, "https://example.com");
    
    std::cout << "Title: " << result.title << "\n";
    // Output: Title: My Page
    
    std::cout << "Content:\n" << result.text << "\n";
    // Output:
    // # Welcome
    // This is **important** text.
    // ```
    // const x = 42;
    // ```
    
    return 0;
}
```

### Пример 2: Работа с очередью
```cpp
#include "rocksdb_manager.h"
#include <iostream>

int main() {
    RocksDBManager db("/tmp/crawler.db");
    db.init();
    
    // Добавить URLs
    db.enqueue_url("https://example.com");
    db.enqueue_url("https://example.com/page1");
    db.enqueue_url("https://example.com/page2");
    
    // Обработать очередь
    while (db.has_queued_urls()) {
        std::string url = db.dequeue_url();
        std::cout << "Processing: " << url << "\n";
        db.mark_visited(url);
    }
    
    // Результаты
    std::cout << "Visited: " << db.get_visited_count() << "\n";
    // Output: Visited: 3
    
    return 0;
}
```

---

## ⚙️ Конфигурация Сборки

### CMakeLists.txt Обновления

```cmake
# Зависимости v3.0
find_package(RocksDB REQUIRED)
# Примечание: gumbo линкуется напрямую через -lgumbo

# Добавить в все target'ы
target_link_libraries(crawler PRIVATE rocksdb)
target_link_libraries(crawler PRIVATE gumbo)

# Источники
add_executable(crawler
    src/main.cpp
    src/rocksdb_manager.cpp
    src/text_extractor.cpp
    # ...существующие файлы...
)
```

---

## 🎯 Последующие Фазы (v3.1+)

### Фаза 2: Integration
- [ ] Интегрировать RocksDB в WebCrawler::crawl_urls()
- [ ] Использовать TextExtractor для всех fetched страниц
- [ ] Хранить extracted text вместо raw HTML

### Фаза 3: Optimization  
- [ ] Batching операций в RocksDB
- [ ] Параллельный parsing с thread pool
- [ ] Индексирование для быстрого поиска

### Фаза 4: Features
- [ ] Metadata extraction (schema.org, OpenGraph)
- [ ] Image processing (alt text, dimensions)
- [ ] Table parsing (CSV export)

---

## 📝 Логирование

Все компоненты используют Logger::instance():

```
2026-01-25T11:01:20.796Z INFO   RocksDB: Database opened successfully
2026-01-25T11:01:34.374Z INFO   TextExtractor: Extracted text from HTML: 65 chars
```

Отключить в logger.h или использовать log level filtering.

---

## 🐛 Отладка

### RocksDB Проблемы
```bash
# Проверить БД файлы
ls -la /path/to/db/

# Очистить и пересоздать
rm -rf /path/to/db && ./test_rocksdb
```

### Text Extraction Проблемы
```cpp
// Добавить debug logging
std::cout << "HTML Length: " << html.length() << "\n";
std::cout << "Extracted: " << result.text.length() << " chars\n";
```

---

## ✅ Чеклист Готовности

- [x] RocksDB Manager реализован и протестирован
- [x] Text Extractor реализован и протестирован
- [x] Все старые тесты проходят (74 тесты)
- [x] Новые тесты проходят (10 тестов)
- [x] Сборка без ошибок (0 errors)
- [x] Documentation создана
- [x] Примеры подготовлены

**Статус: ГОТОВО К ИСПОЛЬЗОВАНИЮ** ✅

---

Создано: 2026-01-25  
Версия: v3.0  
Разработчик: GitHub Copilot
