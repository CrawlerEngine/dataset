# Тестирование с Bazel и gTest

Полное руководство по запуску и написанию тестов для Dataset Crawler.

## Введение в gTest

Google Test (gTest) - это мощный фреймворк для юнит-тестирования на C++. Проект использует:
- **TEST_F**: тесты с fixtures (подготовка/очистка)
- **ASSERT/EXPECT**: различные уровни проверок
- **Bazel integration**: автоматическая компиляция и запуск

## Структура тестов

```
tests/
├── rocksdb_test.cc           # RocksDB тесты (7 тестов)
├── text_extractor_test.cc    # TextExtractor тесты (7 тестов)
├── robots_ua_priority_test.cc     # RobotsUAPriority тесты (4 теста)
├── robots_integration_test.cc     # RobotsIntegration тесты (3 теста)
└── robots_wildcard_test.cc        # RobotsWildcard тесты (9 тестов)
```

**Всего: 30 тестов в 5 файлах**

## Запуск тестов

### Все тесты

```bash
bazel test //...
```

### Конкретный набор тестов

```bash
# Только RocksDB тесты
bazel test //:rocksdb_test

# Только Text Extractor тесты
bazel test //:text_extractor_test

# Тесты robots.txt парсинга
bazel test //:robots_ua_priority_test
bazel test //:robots_integration_test
bazel test //:robots_wildcard_test
```

### Детальный вывод

```bash
# Полный лог всех тестов
bazel test //... --test_output=all

# Только ошибки
bazel test //... --test_output=errors

# Только резюме (по умолчанию)
bazel test //... --test_output=summary
```

## Описание тестовых наборов

### 1. RocksDB Manager Tests (7 тестов)

**Файл:** `tests/rocksdb_test.cc`

Тестирует функциональность RocksDB-based persistence:

| Тест | Описание |
|------|---------|
| DatabaseInitialization | Инициализация БД и проверка статуса |
| EnqueueAndDequeue | FIFO очередь URLs (enqueue/dequeue) |
| QueueSize | Отслеживание размера очереди |
| VisitedTracking | Отслеживание посещённых URL (visited URLs) |
| HTMLCaching | Кэширование HTML ответов |
| Persistence | Сохранение данных между сессиями |
| Statistics | Генерация статистики (счётчики, метрики) |

**Особенность:** Использует уникальные временные директории для каждого теста (避免 конфликты блокировки БД).

```cpp
class RocksDBManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создать уникальную директорию с временной меткой
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        db_path = "/tmp/test_crawler_db_" + std::to_string(getpid()) + "_" + std::to_string(now);
        db = std::make_unique<RocksDBManager>(db_path);
        ASSERT_TRUE(db->init());
    }
};
```

### 2. Text Extractor Tests (7 тестов)

**Файл:** `tests/text_extractor_test.cc`

Тестирует парсинг HTML и извлечение текста с Markdown форматированием:

| Тест | Описание |
|------|---------|
| HeadingConversion | Конвертация H1-H6 в Markdown (#, ##, ###) |
| CodeBlockDetection | Обнаружение `<pre><code>` блоков |
| TextFormatting | Bold, italic, links в тексте |
| ElementRemoval | Удаление nav, footer, script элементов |
| PlainTextExtraction | Извлечение чистого текста |
| LanguageDetection | Определение языка кода в блоках |
| SpecialCharacters | Обработка спецсимволов (& < >) |

### 3. Robots.txt UA Priority Tests (4 теста)

**Файл:** `tests/robots_ua_priority_test.cc`

Тестирует приоритизацию правил robots.txt по User-Agent:

| Тест | Описание |
|------|---------|
| ExactUserAgentMatch | Точное совпадение специфичного user-agent |
| WildcardFallback | Fallback на `*` user-agent при отсутствии точного совпадения |
| AllowOverride | Проверка что Allow переопределяет Disallow |
| ComplexRules | Комплексные правила с мультипл условиями |

### 4. Robots.txt Integration Tests (3 теста)

**Файл:** `tests/robots_integration_test.cc`

Интеграционные тесты полного парсинга robots.txt:

| Тест | Описание |
|------|---------|
| BasicRobotsParsing | Парсинг базовых Disallow правил |
| MultipleRules | Несколько User-Agent секций |
| AllowAndDisallow | Комбинация Allow и Disallow |

### 5. Robots.txt Wildcard Tests (9 тестов)

**Файл:** `tests/robots_wildcard_test.cc`

Тестирует wildcard пат matching (*, $):

| Тест | Описание |
|------|---------|
| WildcardStar | `*.pdf` - звёздочка в пути |
| WildcardEndOfUrl | `/path$` - конец URL |
| AdminDirectoryBlock | `/admin/*` - блокировка директории |
| LongestMatchWins | Самое специфичное правило побеждает |
| AllowBeatDisallow | Allow переопределяет Disallow |
| CaseSensitive | Патчи чувствительны к регистру |
| SpecialCharactersInPath | Спецсимволы в пути (`.cgi$`) |
| EscapeSequences | Экранирование спецсимволов |
| EdgeCases | Граничные случаи и пустые пути |

## Написание собственных тестов

### Структура gTest теста

```cpp
#include <gtest/gtest.h>
#include "my_header.h"

// Тестовый класс (fixture)
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Подготовка перед каждым тестом
        resource = new MyResource();
    }
    
    void TearDown() override {
        // Очистка после каждого теста
        delete resource;
    }
    
    MyResource* resource;
};

// Тест с fixture
TEST_F(MyTest, TestName) {
    // Arrange
    int input = 5;
    
    // Act
    int result = resource->process(input);
    
    // Assert
    ASSERT_EQ(result, 10);  // ASSERT - тест падает при failure
    EXPECT_EQ(result, 10);  // EXPECT - тест продолжается при failure
}
```

### Основные assertions

```cpp
// Равенство
ASSERT_EQ(actual, expected);     // Assert Equal
EXPECT_EQ(actual, expected);     // Expect Equal
EXPECT_NE(actual, expected);     // Not Equal
EXPECT_LT(actual, expected);     // Less Than
EXPECT_GT(actual, expected);     // Greater Than

// Boolean
ASSERT_TRUE(condition);
EXPECT_FALSE(condition);

// Строки
EXPECT_STREQ(str1, str2);        // C-string сравнение
EXPECT_EQ(string1, string2);     // std::string сравнение

// Исключения
EXPECT_THROW(statement, ExceptionType);
EXPECT_NO_THROW(statement);

// Пользовательские сообщения
EXPECT_EQ(a, b) << "Custom message";
```

## Пример: Добавление нового теста

### Шаг 1: Создать тест файл

```cpp
// tests/my_feature_test.cc
#include "my_feature.h"
#include <gtest/gtest.h>

class MyFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        feature = new MyFeature();
    }
    
    void TearDown() override {
        delete feature;
    }
    
    MyFeature* feature;
};

TEST_F(MyFeatureTest, TestPositiveCase) {
    EXPECT_EQ(feature->calculate(5), 10);
}

TEST_F(MyFeatureTest, TestNegativeCase) {
    EXPECT_EQ(feature->calculate(-5), -10);
}
```

### Шаг 2: Добавить в BUILD

```starlark
# В файле BUILD добавить:
cc_test(
    name = "my_feature_test",
    srcs = ["tests/my_feature_test.cc"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)

# И в test_suite:
test_suite(
    name = "all_tests",
    tests = [
        # ... другие тесты ...
        ":my_feature_test",  # Добавить это
    ],
)
```

### Шаг 3: Запустить тест

```bash
bazel test //:my_feature_test
```

## Отладка тестов

### Запуск с полным выводом

```bash
bazel test //:rocksdb_test --test_output=all
```

### Запуск с GDB отладчиком

```bash
bazel build //:rocksdb_test
gdb ./bazel-bin/rocksdb_test
```

### Очистка и пересборка

```bash
bazel clean
bazel test //...
```

### Фильтр по названию теста

```bash
# Запустить только DatabaseInitialization тест
bazel test //:rocksdb_test --test_filter="*DatabaseInitialization*"

# Запустить тесты содержащие "Wildcard"
bazel test //... --test_filter="*Wildcard*"
```

## Performance тестирование

### Запуск тестов с timing

```bash
bazel test //... --test_output=all 2>&1 | grep -E "^\[|ms"
```

### Временные лимиты

```starlark
# В BUILD файле можно установить timeout:
cc_test(
    name = "slow_test",
    srcs = ["tests/slow_test.cc"],
    timeout = "long",  # short, moderate, long, eternal
)
```

## Интеграция с CI/CD

### GitHub Actions пример

```yaml
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: |
          sudo apt-get update
          sudo apt-get install -y bazel librocksdb-dev libgumbo-dev libcurl4-openssl-dev
      - run: bazel test //...
```

## Файлы документации

- [BUILDING.md](./BUILDING.md) - Инструкции по сборке
- [BUILD файл](../BUILD) - Определения целевых объектов
- [WORKSPACE](../WORKSPACE) - Конфигурация зависимостей
