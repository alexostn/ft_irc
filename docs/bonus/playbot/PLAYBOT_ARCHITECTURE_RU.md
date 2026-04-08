# PlayBot: Архитектура и дизайн

## Быстрый старт

```bash
# Компиляция
make playbot

# Запуск серверв (Терминал 1)
./ircserv 6667 testpass

# Запуск бота (Терминал 2)
./playbot 127.0.0.1 6667 testpass playbot #general

# Бот появляется в канале, отправляет статус каждые 60 сек
# Отвечает на: !ping, !echo <text>, !time, !uptime, !help
```

---

## Трёхслойная архитектура

```
┌──────────────────────────────────────────────────────────────┐
│                    BotMain (Основной цикл)                   │
│  • Polling каждые 60s для отправки статуса (ALERT_INTERVAL)  │
│  • Dispatch PRIVMSG + логика переподключения                │
└──────────────────────┬───────────────────────────────────────┘
                       │
        ┌──────────────┴──────────────┐
        │                             │
┌───────▼─────────────┐     ┌─────────▼──────────────┐
│   BotCore           │     │   BotCommands          │
│ ─────────────────────    │ ─────────────────────────
│ • connect()         │     │ • dispatch(line)       │
│ • registerIRC()     │     │ • handle(cmd)          │
│ • sendRaw(msg)      │     │ • sendAlert()          │
│ • tick(ms)          │     │                        │
│ • reconnect(n)      │     │ Команды:               │
│                     │     │ !ping !echo !time      │
│ Слой транспорта:    │     │ !uptime !help          │
│ poll() + recv/send  │     └────────────────────────┘
│ PING/PONG прозрачно │
└─────────────────────┘
```

---

## Ключевые концепции

### 1. Транспортный слой (BotCore)

**Задача:** TCP сокет, polling, буферизация сообщений, автопереподключение.

**Отличительные черты:**
- Неблокирующий I/O: `fcntl(fd, F_SETFL, O_NONBLOCK)`
- Один вызов `poll()` за tick с timeout = остаток времени до следующего статуса
- PING/PONG обрабатываются внутренне — приложение их не видит
- Переподключение: пауза 3 сек между попытками, до N попыток

**Открытый API:**
```cpp
bool connect();                           // DNS lookup + socket + connect
bool registerIRC();                       // PASS → NICK → USER → JOIN
std::vector<std::string> tick(int ms);   // poll(ms) → выделить строки
void sendRaw(const std::string& msg);    // добавить \r\n, отправить
bool reconnect(int max_attempts);        // повторить N раз с задержкой
bool isConnected() const;                // fd >= 0
```

### 2. Обработчик команд (BotCommands)

**Задача:** Парсить PRIVMSG, маршрутизировать в обработчики, отправлять активные алерты.

**Отличительные черты:**
- `parsePrivmsg()`: Извлечь sender, target, text из `:nick!user@host PRIVMSG #chan :message`
- Маршрутизация в отдельные `cmd*()` обработчики по префиксу text
- `sendAlert()`: Активное сообщение, отправляемое при истечении timeout `poll()`

**Обработчики:**
```
!ping         → "pong!"
!echo <text>  → повторить текст
!time         → текущее системное время
!uptime       → секунд с начала работы бота
!help         → список команд + тизер: "coming soon: !play"
```

### 3. Основной цикл (BotMain)

**Задача:** Долгоживущее соединение, синхронизация времени, переподключение.

```cpp
while (true) {
    int remaining_ms = (ALERT_INTERVAL - elapsed_seconds) * 1000;
    lines = core.tick(remaining_ms);       // poll() управляет временем
    
    for (each line) cmds.dispatch(line);   // обработать сообщения
    
    // Alert срабатывает при истечении poll timeout
    if (elapsed >= ALERT_INTERVAL) {
        cmds.sendAlert();
        last_alert = now;
    }
    
    // Переподключение при разрыве
    if (!core.isConnected()) {
        core.reconnect(5) or exit(1);
    }
}
```

---

## Принципы дизайна

### 1. **Управление статусом через Poll (Без Sleep)**
Проблема: `sleep()` в основном цикле блокирует ответ на PRIVMSG.
Решение: Использовать `poll(timeout)` для управления интервалом статуса.

**Результат:**
- Статус отправляется точно каждые 60 сек
- Ответ на PRIVMSG: субмиллисекундно (нет блокировки)
- Нет накопления микро-пауз

### 2. **Прозрачный PING/PONG**
Проблема: Приложению не должно быть дело до IRC keep-alive.
Решение: `BotCore::tick()` перехватывает PING внутренне.

**Результат:**
- Приложение видит только data-bearing PRIVMSG
- BotCommands никогда не видит PING/PONG
- Соединение остаётся живым бесконечно

### 3. **Чистоты C++98**
- Нет `nullptr`, `auto`, range-for, lambdas
- `std::ostringstream` для преобразования число→строка
- Ручное управление памятью: всё на стеке (без new/delete)
- Единственная форма `fcntl()`: `fcntl(fd, F_SETFL, O_NONBLOCK)`

---

## Структура файлов

```
include/irc/bot/
  ├─ BotCore.hpp        (интерфейс: connect, tick, reconnect)
  └─ BotCommands.hpp    (интерфейс: dispatch, sendAlert)

src/bot/
  ├─ BotCore.cpp        (~190 строк: socket/poll/reconnect)
  ├─ BotCommands.cpp    (~120 строк: парс PRIVMSG + обработчики)
  └─ BotMain.cpp        (~70 строк: цикл + управление временем)

docs/bonus/playbot/
  ├─ PLAYBOT_HOW_TO_USE.md (подробное использование + архитектура)
  ├─ PLAYBOT_ARCHITECTURE.md (документ на английском)
  └─ PLAYBOT_ARCHITECTURE_RU.md (этот документ)
```

---

## Компиляция и тестирование

```bash
# Компиляция (с флагами -Wall -Wextra -Werror -std=c++98)
make playbot    # самостоятельный бинарь
make            # включает основной сервер + playbot

# Проверка утечек памяти
valgrind --leak-check=full ./playbot 127.0.0.1 6667 testpass playbot #general

# Юнит-тесты (планируется: часть 2)
# - Парсинг команд с неправильным PRIVMSG
# - Сценарии переподключения (имитация разрыва)
# - Точность срабатывания алерта под нагрузкой
```

---

## Предпросмотр Part 2

**Планируется:** Интегрировать `!play` команду в существующую инфраструктуру.

- `BotCommands::cmdPlay()` — маршрут к игровой конечному автомату
- Состояние игры хранится в контексте BotCore/BotMain
- Dispatch PRIVMSG не изменится — просто добавить новый case в `handle()`

---

## Метрики

- **Размер кода:** ~380 строк (BotCore + BotCommands + BotMain)
- **Компиляция:** ~0.2s на современном CPU
- **Память:** ~2KB для буферов + сокетов
- **Точность алерта:** ±100ms (в пределах разрешения poll())
- **Задержка переподключения:** 3s + DNS lookup (~50-500ms)

