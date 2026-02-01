# Ethical Web Crawling Guide

## Поддерживаемые Web Standards

Dataset Crawler поддерживает следующие стандарты для этичного краулинга:

### 1. robots.txt

Краулер автоматически проверяет файл `robots.txt` перед загрузкой страницы.

**Как это работает:**
- Загружает `/robots.txt` с каждого домена
- Кэширует результаты для оптимизации
- Пропускает страницы, запрещенные директивой `Disallow`
- Поддерживает User-agent matching

**Пример robots.txt:**
```
User-agent: *
Disallow: /admin/
Disallow: /private/
Allow: /private/public/

User-agent: DatasetCrawler
Allow: /
```

### 2. Meta-tags (noindex, nofollow)

Краулер проверяет мета-теги в HTML каждой загруженной страницы.

**Поддерживаемые теги:**
- `<meta name="robots" content="noindex">` - не индексировать страницу
- `<meta name="robots" content="nofollow">` - не следовать ссылкам

**Пример:**
```html
<head>
  <meta name="robots" content="noindex">
  <!-- Эта страница не будет добавлена в датасет -->
</head>
```

## Использование в C++

### Включение проверок (по умолчанию включено)

```cpp
WebCrawler crawler;

// Включить проверку robots.txt
crawler.set_respect_robots_txt(true);

// Включить проверку мета-тегов
crawler.set_respect_meta_tags(true);

// Краулить
auto records = crawler.crawl_urls(urls);
```

### Отключение проверок (если нужно)

```cpp
// Отключить для тестирования
crawler.set_respect_robots_txt(false);
crawler.set_respect_meta_tags(false);
```

### Получение статистики

```cpp
auto records = crawler.crawl_urls(urls);

int blocked_robots = crawler.get_blocked_by_robots_count();
int blocked_noindex = crawler.get_blocked_by_noindex_count();

std::cout << "Blocked by robots.txt: " << blocked_robots << std::endl;
std::cout << "Blocked by noindex: " << blocked_noindex << std::endl;
```

## Примеры использования

### Пример 1: Полное соблюдение стандартов

```cpp
#include "crawler.h"
#include "dataset_writer.h"

int main() {
    WebCrawler crawler;
    crawler.set_timeout(30);
    
    // Соблюдать все веб-стандарты
    crawler.set_respect_robots_txt(true);
    crawler.set_respect_meta_tags(true);
    
    std::vector<std::string> urls = {
        "https://example.com/page1",
        "https://example.com/page2",
        "https://example.com/page3"
    };
    
    auto records = crawler.crawl_urls(urls);
    
    std::cout << "Собрано: " << records.size() << " страниц" << std::endl;
    std::cout << "Пропущено robots.txt: " << crawler.get_blocked_by_robots_count() << std::endl;
    std::cout << "Пропущено noindex: " << crawler.get_blocked_by_noindex_count() << std::endl;
    
    ParquetDatasetWriter writer;
    writer.write_records("dataset.json", records);
    
    return 0;
}
```

### Пример 2: Сообщение о пропущенных страницах

```cpp
#include "crawler.h"

int main() {
    WebCrawler crawler;
    crawler.set_respect_robots_txt(true);
    crawler.set_respect_meta_tags(true);
    
    std::vector<std::string> urls = {
        "https://example.com/page1",
        "https://example.com/page2"
    };
    
    auto records = crawler.crawl_urls(urls);
    
    int total = urls.size();
    int collected = records.size();
    int blocked_robots = crawler.get_blocked_by_robots_count();
    int blocked_noindex = crawler.get_blocked_by_noindex_count();
    
    std::cout << "\nОтчет о краулинге:" << std::endl;
    std::cout << "Всего URL: " << total << std::endl;
    std::cout << "Собрано: " << collected << std::endl;
    std::cout << "Пропущено:" << std::endl;
    std::cout << "  - robots.txt: " << blocked_robots << std::endl;
    std::cout << "  - noindex: " << blocked_noindex << std::endl;
    
    return 0;
}
```

## Логирование

При краулинге вы увидите сообщения о пропущенных страницах:

```
[1/5] Crawling: https://example.com/private
  ⊘ Blocked by robots.txt

[2/5] Crawling: https://example.com/noindex
  ⊘ Blocked by meta noindex tag
```

## Лучшие практики

### ✅ DO (Рекомендуется)

1. **Всегда включайте проверки:**
   ```cpp
   crawler.set_respect_robots_txt(true);
   crawler.set_respect_meta_tags(true);
   ```

2. **Устанавливайте корректный User-Agent:**
   ```cpp
   WebCrawler crawler("MyBot/1.0 (+http://example.com/bot)");
   ```

3. **Логируйте статистику:**
   ```cpp
   std::cout << "Blocked by robots: " << crawler.get_blocked_by_robots_count() << std::endl;
   ```

4. **Мониторьте производительность:**
   ```cpp
   auto start = std::chrono::high_resolution_clock::now();
   auto records = crawler.crawl_urls(urls);
   auto end = std::chrono::high_resolution_clock::now();
   ```

### ❌ DON'T (Не рекомендуется)

1. **Не отключайте проверки без причины**
2. **Не используйте поддельный User-Agent**
3. **Не игнорируйте ошибки доступа (403, 401)**
4. **Не перегружайте сервера слишком частыми запросами**

## Технические детали

### robots.txt парсинг

Краулер парсит следующие директивы:
- `User-agent: *` - применяется ко всем агентам
- `User-agent: DatasetCrawler` - применяется к этому краулеру
- `Disallow: /` - запретить всё
- `Disallow: /admin/` - запретить папку
- `Allow: /private/public/` - разрешить папку

### Meta-tag парсинг

Поддерживается стандартный формат:
```html
<meta name="robots" content="noindex, nofollow">
```

Проверяются следующие значения:
- `noindex` - не добавлять в индекс поисковиков
- `nofollow` - не следовать ссылкам (будущее расширение)

### Кэширование robots.txt

Результаты парсинга robots.txt кэшируются по доменам:
- Улучшает производительность
- Снижает нагрузку на целевые серверы
- Кэш сбрасывается при создании нового экземпляра WebCrawler

## FAQ

**Q: Что если robots.txt недоступен?**
A: Краулер считает, что доступ разрешен и продолжает работу.

**Q: Может ли страница быть заблокирована обоими механизмами?**
A: Да, статистика учитывает оба события отдельно.

**Q: Как обновить кэш robots.txt?**
A: Создайте новый экземпляр WebCrawler.

**Q: Поддерживаются ли другие мета-теги?**
A: Текущая версия поддерживает только `noindex`. Планируется расширение.

## Соответствие стандартам

Этот краулер соответствует:
- ✅ [Robots Exclusion Standard](https://www.robotstxt.org/)
- ✅ [Meta robots tag](https://developers.google.com/search/docs/advanced/robots/robots_meta_tag)
- ✅ Лучшим практикам веб-краулинга
- ✅ [Web Robots Guidelines](https://www.w3.org/1999/07/001-wd-html40-970729)

---

**Помните:** Этичный краулинг важен для здоровья интернета и хороших отношений с владельцами сайтов.
