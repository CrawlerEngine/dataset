# Сборка проекта с Bazel

Руководство по сборке Dataset Crawler с использованием современной системы сборки Bazel.

## Требования

- **Bazel** 6.0 или выше
- **C++17** совместимый компилятор (GCC 7+, Clang 5+)
- **Linux/macOS** (Windows требует WSL или MSYS2)
- **Системные библиотеки:**
  - `librocksdb-dev` (RocksDB)
  - `libgumbo-dev` (HTML парсер)
  - `libcurl4-openssl-dev` (HTTP клиент)

## Установка Bazel

### Linux/macOS (рекомендуется)

```bash
# Ubuntu/Debian
sudo apt-get install bazel

# или скачать бинарник напрямую
wget https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel-6.4.0-linux-x86_64
chmod +x bazel-6.4.0-linux-x86_64
sudo mv bazel-6.4.0-linux-x86_64 /usr/local/bin/bazel

# Проверить установку
bazel --version
```

### Установка зависимостей (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
  librocksdb-dev \
  libgumbo-dev \
  libcurl4-openssl-dev
```

## Базовые команды

### Сборка всех целевых объектов

```bash
bazel build //...
```

Результат: все бинарники и библиотеки компилируются в `bazel-bin/`.

### Сборка только main binary

```bash
bazel build //:crawler
```

Результат: `bazel-bin/crawler` (готовый исполняемый файл)

### Запуск всех тестов

```bash
bazel test //...
```

**Результат:** 30 gTest тестов, распределённые в 5 файлов:
- rocksdb_test (7 тестов)
- text_extractor_test (7 тестов)
- robots_ua_priority_test (4 теста)
- robots_integration_test (3 теста)
- robots_wildcard_test (9 тестов)

### Запуск конкретного теста

```bash
# Запустить только RocksDB тесты
bazel test //:rocksdb_test

# Запустить с детальным выводом
bazel test //:rocksdb_test --test_output=all
```

### Детальный вывод тестов

```bash
# Все тесты с полным логом
bazel test //... --test_output=all

# Только ошибки
bazel test //... --test_output=errors

# Краткое резюме (по умолчанию)
bazel test //... --test_output=summary
```

## Конфигурация Bazel

### Файл `.bazelrc`

Содержит глобальные параметры сборки:

```bazelrc
# C++ standard
build --cxxopt=-std=c++17

# Optimization level
build -c opt

# Test options
test --test_output=summary

# Parallel compilation
build --jobs=auto

# Compiler warnings
build --cxxopt=-Wall
build --cxxopt=-Wextra
```

### WORKSPACE файл

Определяет монорепозиторий и внешние зависимости:

```starlark
workspace(name = "dataset_crawler")

# Google Test (gTest) dependency
http_archive(
    name = "com_google_googletest",
    sha256 = "8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7",
    strip_prefix = "googletest-1.14.0",
    urls = ["https://github.com/google/googletest/archive/v1.14.0.tar.gz"],
)
```

### BUILD файл

Содержит определения целевых объектов (targets):

```starlark
# Main executable
cc_binary(
    name = "crawler",
    srcs = ["src/main.cpp"],
    deps = [":crawler_lib"],
    copts = ["-std=c++17"],
)

# Library target
cc_library(
    name = "crawler_lib",
    srcs = glob(["src/**/*.cpp"], exclude = ["src/main.cpp"]),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    copts = ["-std=c++17"],
    linkopts = ["-lrocksdb", "-lgumbo", "-lcurl"],
)

# Test targets
cc_test(
    name = "rocksdb_test",
    srcs = ["tests/rocksdb_test.cc"],
    deps = [":crawler_lib", "@com_google_googletest//:gtest_main"],
)
```

## Структура Bazel файлов

```
/workspaces/dataset/
├── WORKSPACE                    # Конфигурация монорепо
├── BUILD                        # Основные цели сборки
├── .bazelrc                     # Параметры компиляции
├── .bazelignore                 # Исключаемые файлы
├── bazel/
│   └── external/
│       ├── rocksdb.BUILD        # RocksDB конфигурация
│       ├── gumbo.BUILD          # Gumbo конфигурация
│       └── curl.BUILD           # CURL конфигурация
├── src/                         # Исходные файлы
├── include/                     # Заголовочные файлы
└── tests/                       # Тестовые файлы (gTest)
```

## Оптимизация сборки

### Кэширование

Bazel автоматически кэширует результаты сборки:

```bash
# Очистить кэш
bazel clean

# Вывести статистику кэша
bazel query ...
```

### Параллельная сборка

```bash
# Использовать все CPU ядра (по умолчанию)
bazel build //... -j auto

# Использовать конкретное количество ядер
bazel build //... -j 4
```

### Отключение оптимизации (для debug)

```bash
# Debug сборка
bazel build //... -c dbg

# Оптимизированная сборка (по умолчанию)
bazel build //... -c opt
```

## Отладка

### Просмотр параметров сборки

```bash
# Показать команду компиляции
bazel build //... --verbose_failures

# С песочницей (sandbox)
bazel build //... --sandbox_debug
```

### Просмотр целей

```bash
# Все цели в проекте
bazel query "//..."

# Только тесты
bazel query "kind(cc_test, //...)"

# Зависимости целевого объекта
bazel query "deps(//:crawler)"
```

## Часто встречаемые ошибки

### Ошибка: "cannot find -lgumbo"

**Решение:** Установить системные библиотеки:
```bash
sudo apt-get install libgumbo-dev librocksdb-dev libcurl4-openssl-dev
```

### Ошибка: "Checksum mismatch"

**Решение:** Обновить checksum в WORKSPACE файле или удалить кэш:
```bash
rm -rf ~/.cache/bazel
bazel build //...
```

### Ошибка: "Java not found"

**Решение:** Bazel требует Java Runtime Environment:
```bash
sudo apt-get install openjdk-11-jre-headless
```

## Миграция с CMake

Проект всё ещё содержит `CMakeLists.txt` для обратной совместимости, но рекомендуется использовать Bazel:

### Старый способ (CMake)
```bash
mkdir build
cd build
cmake ..
make
./crawler
```

### Новый способ (Bazel)
```bash
bazel build //:crawler
./bazel-bin/crawler
```

## Версионирование

- **v3.0+:** Bazel + gTest (текущая версия)
- **v2.7:** CMake + Custom tests (legacy)

## Документация

- [WORKSPACE конфигурация](../WORKSPACE)
- [BUILD файлы](../BUILD)
- [API документация](./API.md)
- [Тестирование](./TESTING.md)
