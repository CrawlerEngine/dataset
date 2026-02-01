# Краулинг своего сайта

## Быстрый старт

### 1. Модифицировать config.json

Отредактируй файл `config.json` и добавь свои URLs:

```json
{
    "crawler": {
        "timeout": 30,
        "max_retries": 3,
        "user_agent": "MyDatasetBot/1.0 (+https://yoursite.com/bot)",
        "follow_redirects": true,
        "respect_robots_txt": true,
        "respect_meta_tags": true
    },
    "output": {
        "format": "both",
        "output_dir": "./output",
        "batch_size": 1000
    },
    "urls": [
        "https://example.com",
        "https://example.com/page1",
        "https://example.com/page2",
        "https://yoursite.com"
    ],
    "headers": {
        "Accept-Language": "en-US,en;q=0.9",
        "Accept-Encoding": "gzip, deflate",
        "Cache-Control": "no-cache"
    }
}
```

Затем запусти:
```bash
cd build
./crawler
```

### 2. Используя command-line аргументы

#### Одиночный URL:
```bash
cd build
./crawler --url "https://yoursite.com"
```

#### Несколько URL:
```bash
cd build
./crawler --urls "https://site1.com,https://site2.com,https://site3.com"
```

#### С дополнительными параметрами:
```bash
cd build
./crawler --urls "https://yoursite.com" --timeout 45 --user-agent "MyBot/1.0"
```

#### Загрузить из конфиг файла:
```bash
cd build
./crawler --config ../config.json
```

## ⚠️ Важные замечания по LinkedIn

**LinkedIn запрещает автоматизированный краулинг!**

Вот что говорят их Terms of Service:
- Использование ботов, скрепинга или автоматизированных инструментов для сбора данных запрещено
- Нарушение может привести к блокировке аккаунта и судебным разбирательствам

### Законные альтернативы для LinkedIn:
1. **LinkedIn API** - официальный способ получить данные
2. **LinkedIn Data Scrape** - платный сервис от LinkedIn
3. **Собрать данные вручную** для малых наборов
4. **Использовать публичные данные** которые LinkedIn специально предоставляет

## Примеры для других сайтов

### Wikipedia (разрешает краулинг)

```json
{
    "urls": [
        "https://en.wikipedia.org/wiki/Machine_learning",
        "https://en.wikipedia.org/wiki/Artificial_intelligence",
        "https://en.wikipedia.org/wiki/Data_science"
    ]
}
```

```bash
./crawler --urls "https://en.wikipedia.org/wiki/Machine_learning,https://en.wikipedia.org/wiki/Artificial_intelligence"
```

### GitHub (разрешает краулинг)

```bash
./crawler --urls "https://github.com/trending,https://github.com/explore"
```

### ArXiv (бесплатный доступ к научным статьям)

```bash
./crawler --urls "https://arxiv.org/list/cs.AI/recent,https://arxiv.org/list/stat.ML/recent"
```

### Stack Overflow (разрешает краулинг)

```bash
./crawler --urls "https://stackoverflow.com/questions/tagged/machine-learning,https://stackoverflow.com/questions/tagged/deep-learning"
```

## Проверка robots.txt

Краулер автоматически уважает `robots.txt`. Перед краулингом сайта:

1. Посетите `https://yoursite.com/robots.txt`
2. Проверьте, разрешен ли краулинг для User-Agent: `*` или вашего User-Agent
3. Если сайт запрещает краулинг - соблюдайте это!

### Пример robots.txt:
```
User-agent: *
Disallow: /admin
Disallow: /private

User-agent: DatasetCrawler
Disallow: /temp
Allow: /public
```

## Оптимальные параметры

### Для маленьких сайтов:
```bash
./crawler --url "https://mysite.com" --timeout 15
```

### Для больших сайтов:
```bash
./crawler --url "https://bigsite.com" --timeout 60
```

### С кастомным User-Agent:
```bash
./crawler --url "https://mysite.com" --user-agent "MyBot/1.0 (+https://mysite.com/bot)"
```

## Логирование

Краулер выдает структурированные логи:

```
2026-01-25T08:07:46.072Z INFO   === Dataset Crawler for AI ===
2026-01-25T08:07:46.073Z INFO   Configuration: 4 URLs, timeout: 30s, robots.txt: YES, meta-tags: YES
2026-01-25T08:07:46.122Z INFO   https://example.com [200]
2026-01-25T08:07:46.292Z INFO   Crawling complete. Total records: 4
```

## Вывод

Краулер экспортирует данные в:
- `output/dataset.json` - JSON формат
- `output/dataset.csv` - CSV формат

## Этичный краулинг

Следи за:
1. ✅ robots.txt - краулер это делает автоматически
2. ✅ Meta-теги noindex/nofollow - краулер это делает автоматически
3. ✅ Соответствующий User-Agent - используй `--user-agent`
4. ✅ Разумный timeout - не перегружай сервер
5. ✅ Terms of Service сайта - всегда проверяй перед краулингом

## Примеры скриптов

### Краулить множество публичных сайтов:
```bash
./crawler --urls \
  "https://en.wikipedia.org/wiki/Machine_learning,\
   https://arxiv.org/list/cs.AI/recent,\
   https://github.com/trending"
```

### С полным конфигом:
```bash
./crawler \
  --config ../config.json \
  --timeout 45 \
  --user-agent "AcademicBot/1.0 (+https://university.edu/bot)"
```

## Troubleshooting

### Краулер блокирует страницы из-за robots.txt

Если нужно отключить проверку robots.txt (только для собственных сайтов!):

1. Отредактируй `config.json`:
```json
"respect_robots_txt": false
```

2. Или используй конфиг файл с этой настройкой

### Сервер блокирует краулер

Решения:
1. Увеличь `timeout` в config.json
2. Добавь детальный User-Agent: `--user-agent "MyBot/1.0 (+https://mysite.com)"`
3. Проверь robots.txt - может быть краулинг запрещен

### Слишком медленно

Причины и решения:
1. Увеличь `max_retries` в config.json
2. Уменьши `timeout` если это не критично
3. Проверь скорость интернета
