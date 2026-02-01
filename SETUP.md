# 🚀 Dataset Crawler - Руководство по использованию

## Настройка краулера на свой сайт

### 1️⃣ Способ №1: Command-line аргументы (самый быстрый)

```bash
cd build
./crawler --url "https://yoursite.com"
```

**Примеры:**

```bash
# Один URL
./crawler --url "https://example.com"

# Несколько URL (через запятую)
./crawler --urls "https://site1.com,https://site2.com,https://site3.com"

# С кастомным User-Agent
./crawler --url "https://mysite.com" --user-agent "MyBot/1.0"

# С увеличенным timeout (для медленных сайтов)
./crawler --url "https://mysite.com" --timeout 60

# Все параметры вместе
./crawler --urls "https://site1.com,https://site2.com" --timeout 45 --user-agent "MyBot/1.0"
```

### 2️⃣ Способ №2: Конфиг файл (для больших проектов)

Отредактируй `config.json`:

```json
{
    "crawler": {
        "timeout": 30,
        "max_retries": 3,
        "user_agent": "MyBot/1.0 (+https://yoursite.com/bot)",
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
./crawler --config ../config.json
```

### 3️⃣ Способ №3: Встроенные примеры

```bash
# Запусти скрипт с примерами
bash examples.sh 1     # Single URL
bash examples.sh 2     # Multiple URLs
bash examples.sh 3     # Wikipedia (education)
bash examples.sh 4     # GitHub Trending
bash examples.sh 5     # ArXiv (research papers)
bash examples.sh 6     # Custom User-Agent
bash examples.sh 7     # Increased timeout
bash examples.sh 8     # Config file
```

## 📋 Все параметры команды

| Параметр | Описание | Пример |
|----------|---------|--------|
| `--url` | Одиночный URL для краулинга | `--url "https://example.com"` |
| `--urls` | Несколько URL (через запятую) | `--urls "url1,url2,url3"` |
| `--timeout` | Timeout в секундах (по умолчанию 30) | `--timeout 60` |
| `--user-agent` | Кастомный User-Agent для идентификации | `--user-agent "MyBot/1.0"` |
| `--config` | Путь к конфиг файлу | `--config config.json` |
| `--output-dir` | Директория для выходных файлов | `--output-dir ./data` |

## 🏢 Примеры для популярных сайтов

### ✅ Разрешенные сайты

#### Wikipedia
```bash
./crawler --urls "https://en.wikipedia.org/wiki/Machine_learning,https://en.wikipedia.org/wiki/Artificial_intelligence"
```

#### GitHub
```bash
./crawler --url "https://github.com/trending" --timeout 30
```

#### ArXiv (научные статьи)
```bash
./crawler --url "https://arxiv.org/list/cs.AI/recent" --timeout 20
```

#### Stack Overflow
```bash
./crawler --url "https://stackoverflow.com/questions/tagged/python" --timeout 25
```

#### Medium
```bash
./crawler --url "https://medium.com/tag/artificial-intelligence" --timeout 30
```

### ❌ ЗАПРЕЩЕННЫЕ сайты

#### LinkedIn - СТРОГО ЗАПРЕЩЕНО ⛔
```bash
# НЕ ДЕЛАЙ ЭТО!
./crawler --url "https://linkedin.com"
```

**Почему?** LinkedIn явно запрещает автоматизированный краулинг в своих Terms of Service. Нарушение может привести к:
- 🚫 Блокировке аккаунта
- ⚖️ Судебным искам
- 💰 Гражданским штрафам (от $5,000 до $100,000+ за нарушение CFAA)

**Легальные альтернативы для LinkedIn:**
1. **LinkedIn API** - https://developers.linkedin.com/
2. **LinkedIn Data Download** - личные данные через LinkedIn интерфейс
3. **Kaggle Datasets** - готовые наборы данных с LinkedIn
4. **LinkedIn Research** - официальные исследования LinkedIn

## 📊 Выход

Краулер создает файлы в `output/` директории:

### dataset.json
```json
[
  {
    "url": "https://example.com",
    "title": "Example Domain",
    "content_length": 1256,
    "timestamp": "2026-01-25 08:15:46",
    "status_code": 200
  }
]
```

### dataset.csv
```
url,title,content_length,timestamp,status_code
https://example.com,"Example Domain",1256,"2026-01-25 08:15:46",200
```

## 🔍 Логирование

Краулер выдает структурированные логи с временными метками:

```
2026-01-25T08:15:46.382Z INFO   Configuration: 2 URLs, timeout: 15s
2026-01-25T08:15:46.383Z INFO   Starting the crawler.
2026-01-25T08:15:46.433Z INFO   https://example.com [200]
2026-01-25T08:15:46.480Z INFO   Crawling completed. Fetched: 2 records
```

Цветовая схема:
- 🟢 **INFO** (зелёный) - Успешные операции
- 🟡 **WARN** (жёлтый) - Потенциальные проблемы
- 🔴 **ERROR** (красный) - Критические ошибки
- 🔵 **DEBUG** (голубой) - Детальная диагностика

## ⚙️ Оптимальные параметры

### Для маленьких сайтов
```bash
./crawler --url "https://mysite.com" --timeout 15
```

### Для больших сайтов
```bash
./crawler --url "https://bigsite.com" --timeout 60
```

### Для медленного интернета
```bash
./crawler --url "https://site.com" --timeout 90
```

### Для множества URL
```bash
./crawler --urls "url1,url2,url3,url4,url5" --timeout 30
```

## ✅ Этичный краулинг

Краулер автоматически соблюдает:
- ✅ **robots.txt** - загружает и парсит сразу после получения домена
- ✅ **Meta-tags** - проверяет `<meta name="robots" content="noindex">` 
- ✅ **User-Agent** - позволяет сайтам идентифицировать тебя
- ✅ **Timeout** - не перегружает сервер

Перед краулингом сайта:
1. Посетите `https://yoursite.com/robots.txt`
2. Проверьте, разрешен ли краулинг для User-Agent: `*`
3. Соблюдайте ограничения если они есть

## 🔧 Troubleshooting

| Проблема | Решение |
|----------|---------|
| "Failed to open JSON file" | `mkdir -p build/output` |
| Сайт блокирует краулер | Увеличь timeout: `--timeout 90` |
| Слишком медленно | Проверь интернет-соединение |
| robots.txt блокирует | Это правильно! Соблюдай эти правила |
| 403/429 статус код | Сайт ограничивает доступ (это нормально) |

## 📚 Дополнительная документация

- [USAGE.md](./USAGE.md) - Полное руководство по использованию
- [LOGGING.md](./LOGGING.md) - Документация по логированию
- [ETHICS.md](./ETHICS.md) - Этичный краулинг и robots.txt
- [README.md](./README.md) - API и архитектура

## 🚀 Примеры скриптов

### Краулить список сайтов в цикле
```bash
#!/bin/bash
for site in "https://site1.com" "https://site2.com" "https://site3.com"; do
    ./crawler --url "$site" --timeout 30
    sleep 2  # Пауза между краулингами
done
```

### Краулить с сохранением в разные файлы
```bash
sites=("https://example.com" "https://wikipedia.org")
for site in "${sites[@]}"; do
    domain=$(echo "$site" | cut -d'/' -f3)
    ./crawler --url "$site" --output-dir "./data/$domain"
done
```

## ⚖️ Юридические замечания

Перед краулингом любого сайта:
1. ✅ Прочитай Terms of Service сайта
2. ✅ Проверь robots.txt файл
3. ✅ Убедись что краулинг разрешен
4. ✅ Используй правильный User-Agent
5. ✅ Не переделай сервер (используй разумный timeout)
6. ✅ Соблюдай авторские права на содержимое

## 📞 Поддержка

Если у тебя есть проблемы:
1. Проверь логи - они очень информативны
2. Прочитай LOGGING.md для понимания логов
3. Посмотри примеры в examples.sh
4. Проверь config.json валидность

---

**Помни:** Всегда краулируй ответственно! 🙏
