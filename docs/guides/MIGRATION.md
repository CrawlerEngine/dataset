# Миграция с CMake на Bazel

Руководство по переходу от CMake к современной системе сборки Bazel.

## Почему Bazel?

| Аспект | CMake | Bazel |
|--------|-------|-------|
| **Скорость сборки** | Medium - полная пересборка медленнее | Fast - инкрементальные сборки оптимизированы |
| **Кэширование** | Базовое - only time-based | Продвинутое - content-addressable |
| **Тестирование** | Külön runner нужен | Встроенное, распределённое |
| **Масштабируемость** | OK для малых/средних проектов | Отлично для моно-репозиториев |
| **Воспроизводимость** | Может отличаться на разных машинах | Гарантированно одинаково везде |
| **Параллелизм** | Ручная настройка | Автоматический |
| **Цена обучения** | Низкая | Medium - но долгосрочная выгода выше |

## Структура миграции

### CMake (старая система)

```cmake
cmake_minimum_required(VERSION 3.20)
project(dataset_crawler)

set(CMAKE_CXX_STANDARD 17)

add_executable(crawler
    src/main.cpp
    src/crawler.cpp
    src/rocksdb_manager.cpp
    src/text_extractor.cpp
    # ... другие файлы
)

target_link_libraries(crawler
    rocksdb
    gumbo
    curl
)

# Тесты запускаются отдельным скриптом
add_executable(test_rocksdb tests/test_rocksdb.cpp)
target_link_libraries(test_rocksdb rocksdb)
```

### Bazel (новая система)

```starlark
# WORKSPACE - конфигурация зависимостей
workspace(name = "dataset_crawler")

http_archive(
    name = "com_google_googletest",
    # ...
)

# BUILD - определение целевых объектов
cc_binary(
    name = "crawler",
    srcs = ["src/main.cpp"],
    deps = [":crawler_lib"],
)

cc_library(
    name = "crawler_lib",
    srcs = glob(["src/**/*.cpp"], exclude = ["src/main.cpp"]),
    hdrs = glob(["include/**/*.h"]),
    deps = [/* external deps */],
)

cc_test(
    name = "rocksdb_test",
    srcs = ["tests/rocksdb_test.cc"],
    deps = [":crawler_lib", "@com_google_googletest//:gtest_main"],
)
```

## Шаг за шагом миграция

### 1. Установить Bazel

```bash
# Linux
wget https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel-6.4.0-linux-x86_64
chmod +x bazel-6.4.0-linux-x86_64
sudo mv bazel-6.4.0-linux-x86_64 /usr/local/bin/bazel

# Проверить
bazel --version
```

### 2. Создать WORKSPACE файл

```starlark
# WORKSPACE - точка входа для монорепозитория
workspace(name = "dataset_crawler")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Внешние зависимости
http_archive(
    name = "com_google_googletest",
    sha256 = "8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7",
    strip_prefix = "googletest-1.14.0",
    urls = ["https://github.com/google/googletest/archive/v1.14.0.tar.gz"],
)
```

### 3. Создать BUILD файл

```starlark
# BUILD - определение целевых объектов в пакете
package(default_visibility = ["//visibility:public"])

# Main executable
cc_binary(
    name = "crawler",
    srcs = ["src/main.cpp"],
    deps = [":crawler_lib"],
    copts = ["-std=c++17"],
)

# Shared library (используется тестами)
cc_library(
    name = "crawler_lib",
    hdrs = glob(["include/**/*.h"]),
    srcs = glob(["src/**/*.cpp"], exclude = ["src/main.cpp"]),
    includes = ["include"],
    copts = ["-std=c++17"],
    linkopts = ["-lrocksdb", "-lgumbo", "-lcurl"],
)

# Test targets
cc_test(
    name = "rocksdb_test",
    srcs = ["tests/rocksdb_test.cc"],
    copts = ["-std=c++17"],
    deps = [
        ":crawler_lib",
        "@com_google_googletest//:gtest_main",
    ],
)
```

### 4. Конвертировать тесты к gTest

**Старый стиль (custom framework):**
```cpp
// test_rocksdb.cpp
void test_database_initialization() {
    RocksDBManager db("/tmp/test");
    assert(db.init() == true);
}

int main() {
    test_database_initialization();
    return 0;
}
```

**Новый стиль (gTest):**
```cpp
// tests/rocksdb_test.cc
#include "rocksdb_manager.h"
#include <gtest/gtest.h>

class RocksDBManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<RocksDBManager>("/tmp/test");
        ASSERT_TRUE(db->init());
    }
};

TEST_F(RocksDBManagerTest, DatabaseInitialization) {
    EXPECT_TRUE(db->init());
}
```

### 5. Создать конфигурационные файлы

**.bazelrc** - параметры сборки:
```bazelrc
build --cxxopt=-std=c++17
build -c opt
test --test_output=summary
build --jobs=auto
```

**.bazelignore** - исключаемые директории:
```
CMakeLists.txt
CMakeFiles/
build/
docs/
.git/
```

### 6. Тестировать миграцию

```bash
# Собрать все целевые объекты
bazel build //...

# Запустить тесты
bazel test //...

# Запустить бинарник
./bazel-bin/crawler

# Проверить что работает как раньше
./bazel-bin/crawler --help
```

## Сравнение команд

| Задача | CMake | Bazel |
|--------|-------|-------|
| **Сборка всего** | `cmake build && make` | `bazel build //...` |
| **Сборка одной цели** | `make crawler` | `bazel build //:crawler` |
| **Запуск бинарника** | `./build/crawler` | `./bazel-bin/crawler` |
| **Тесты** | `ctest` или `./test_runner` | `bazel test //...` |
| **Один тест** | `./test_rocksdb` | `bazel test //:rocksdb_test` |
| **Очистка** | `rm -rf build/` | `bazel clean` |
| **Кросс-платформа** | Дополнительно | Встроенная поддержка |

## Особенности Bazel

### Glob patterns

```starlark
# Все .cpp файлы в src, кроме main.cpp
glob(["src/**/*.cpp"], exclude = ["src/main.cpp"])

# Все .h файлы в include
glob(["include/**/*.h"])

# Конкретные файлы
["src/main.cpp", "src/utils.cpp"]
```

### Зависимости

```starlark
# Внутренние зависимости (в том же пакете)
deps = [":crawler_lib"]

# Внешние зависимости (из WORKSPACE)
deps = ["@com_google_googletest//:gtest_main"]

# Системные библиотеки (linkopts)
linkopts = ["-lrocksdb", "-lgumbo", "-lcurl"]
```

### Visibility

```starlark
# Доступна везде
visibility = ["//visibility:public"]

# Доступна только в пакете
visibility = ["//visibility:private"]

# Доступна конкретному пакету
visibility = ["//subdir:__pkg__"]
```

## Отладка миграции

### Проблема: "cannot find -lgumbo"

**Решение 1: Установить системные библиотеки**
```bash
sudo apt-get install libgumbo-dev librocksdb-dev libcurl4-openssl-dev
```

**Решение 2: Использовать внешние BUILD файлы**
```starlark
http_archive(
    name = "gumbo",
    urls = ["https://github.com/google/gumbo-parser/archive/refs/tags/v0.12.0.tar.gz"],
    build_file = "@//bazel:gumbo.BUILD",
)
```

### Проблема: "RocksDB lock hold by current process"

**Решение: Использовать уникальные директории в тестах**
```cpp
auto now = std::chrono::system_clock::now().time_since_epoch().count();
db_path = "/tmp/test_" + std::to_string(getpid()) + "_" + std::to_string(now);
```

### Просмотр параметров компиляции

```bash
bazel build //... --verbose_failures
```

## Обратная совместимость

### Сохранить CMakeLists.txt?

**Плюсы:**
- Старые пользователи могут продолжать использовать CMake
- Постепенный переход
- Дополнительная гибкость

**Минусы:**
- Нужно поддерживать оба система
- Конфликты при обновлениях
- Удвоенная работа

**Рекомендация:** Если проект стабилен, удалить CMakeLists.txt. Если активный, оставить как deprecated.

## Дополнительные ресурсы

- [Official Bazel Documentation](https://bazel.build/docs)
- [Google Test Documentation](https://google.github.io/googletest/)
- [Bazel C++ Guide](https://bazel.build/docs/cpp)
- [BUILD файл синтаксис](https://bazel.build/docs/be/c-cpp)

## Чеклист миграции

- [ ] Установить Bazel
- [ ] Создать WORKSPACE файл
- [ ] Создать основной BUILD файл
- [ ] Конвертировать один тест к gTest
- [ ] Добавить остальные тесты
- [ ] Создать .bazelrc и .bazelignore
- [ ] Протестировать сборку: `bazel build //...`
- [ ] Протестировать тесты: `bazel test //...`
- [ ] Обновить CI/CD для Bazel
- [ ] Обновить документацию
- [ ] Удалить (или архивировать) CMakeLists.txt

## Версионирование

- **v3.0+**: Bazel + gTest (текущая версия)
- **v2.7**: CMake + Custom tests (legacy, archived в docs/v2.7/)
