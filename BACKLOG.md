# BACKLOG — CafeTrainAtmega328

Последнее обновление: 2026-06-22  
Статусы: `[ ]` не начато · `[~]` в работе · `[x]` сделано

## Итоги ревизии кода

| Категория | Количество |
|-----------|-----------|
| Реальные баги | 3 |
| Архитектурные проблемы | 6 |
| Код-смелл | 4 |

---

## 🔴 РЕАЛЬНЫЕ БАГИ

### BUG-01 — `processPWMUpPath1` убивает питание PATH2
**Файл:** `Receiver/scr/PWM.c`, функция `processPWMUpPath1()`  
**Описание:** Когда PATH1 заканчивает разгон, функция безусловно сбрасывает `PWM_PATH1_SWITCH_PIN` — общий силовой ключ для обоих путей. Если в этот момент PATH2 находится в состоянии `ROUTE_SETUP` или `ACCELERATION`, его питание обрывается.  
**Текущий код:**
```c
PORTB &= ~(1 << PWM_PATH1_SWITCH_PIN);  // сбрасывает без проверки PATH2
disablePWMPath(1);
```
**Исправление:** Перед сбросом пина добавить проверку `shouldKeepSharedPathPowerForPath2()` — функция уже существует в `loco_control.c`, просто не используется здесь.  
**Статус:** `[ ]`

---

### BUG-02 — `CMD_SLOW_MODE` и `CMD_IDEL_STOP` определены, но не обрабатываются
**Файл:** `Receiver/main.cpp`, функция `process_packet()` / `Receiver/include/config.h`  
**Описание:** В `config.h` определены команды `CMD_SLOW_MODE 0x22` и `CMD_IDEL_STOP 0x31`, но в `switch(packet.cmd)` для них нет `case`. Если Transmitter когда-либо пошлёт эти команды — Receiver молча проигнорирует.  
**Исправление:** Либо добавить обработчики, либо удалить определения и убрать из протокола как нереализованные.  
**Статус:** `[x]` устранено в коммите после b5ca854

**Что сделано:**
- `Receiver/main.cpp`: добавлен `case CMD_SLOW_MODE` → вызывает `SlowModePath()` для нужного пути + ACK
- `Receiver/main.cpp`: добавлен `case CMD_IDEL_STOP` → полная остановка + ACK (аналог CMD_STOP без сброса аварии)
- `Receiver/main.cpp`: `CMD_IDEL_STOP` добавлен в список команд, проходящих сквозь emergency-фильтр
- `Receiver/scr/protection.c`: `stopLocoDueToTimeout()` теперь шлёт `CMD_IDEL_STOP` на Transmitter (2 раза)
- `Transmitter/main.cpp`: добавлен обработчик `CMD_IDEL_STOP` — сбрасывает статус движения, показывает "TIMEOUT" на LCD

---


### BUG-04 — Мёртвая проверка сенсоров в главном цикле
**Файл:** `Receiver/main.cpp`, основной цикл `while(1)`  
**Описание:** После вызова `checkSensorsState()` переменная `previousSensorStates` уже синхронизирована с `sensorStates`. Следующая проверка `if (sensorStates != previousSensorStates)` никогда не будет истинной — мёртвый код.  
**Исправление:** Удалён блок `if` из главного цикла.  
**Статус:** `[x]`

---

## 🟠 АРХИТЕКТУРНЫЕ ПРОБЛЕМЫ

### ARCH-01 — Три копии одной функции `get_table_path`
**Файлы:** `Receiver/scr/loco_control.c`, `Receiver/scr/railroad_control.c`, `Receiver/main.cpp`  
**Описание:** Одна и та же логика реализована трижды с разными именами, плюс разная система индексации (0-based vs 1-based):
```c
// railroad_control.c — 0-based
static uint8_t get_route_path(uint8_t tableIndex) { return (tableIndex < 4) ? 1 : 2; }
// loco_control.c — 0-based
static uint8_t get_table_path(uint8_t tableIndex)  { return (tableIndex < 4) ? 1 : 2; }
// main.cpp — 1-based
static uint8_t getTablePath(uint8_t tableId) { return (tableId >= 1 && tableId <= 4) ? 1 : 2; }
```
**Исправление:** Вынести одну каноническую функцию в `config.h` как `static inline`, удалить дубликаты.  
**Статус:** `[ ]`

---

### ARCH-02 — `(void)data` в `send_command_with_ack` — ложный API
**Файл:** `Transmitter/uart.c`, функция `send_command_with_ack()`  
**Описание:** Параметр `data` принимается, но немедленно выбрасывается. Вместо него в пакет уходит `seq`. Вызывающий код думает что передаёт данные — это не так.
```c
uint8_t send_command_with_ack(uint8_t cmd, uint8_t table_id, uint8_t data) {
    (void)data;                        // параметр выброшен
    send_command(cmd, table_id, seq);  // уходит seq вместо data
```
**Исправление:** Убрать параметр `data` из сигнатуры, или исправить чтобы `data` реально передавался, обновив ACK-проверку на Receiver (менять оба МК одновременно).  
**Статус:** `[ ]`

---

### ARCH-03 — Хрупкое состояние в `activate_route_non_blocking` через статические локальные переменные
**Файл:** `Receiver/scr/railroad_control.c`  
**Описание:** Состояние автомата маршрута хранится в `static` локальных переменных внутри функции (`lastTick`, `currentBit`, `pulseActive`). Внешний сброс через `reset_route_state()` трогает только глобальный `initialized`, но не эти переменные — они сбрасываются только при следующем вызове функции с `initialized == 0`. Кроме того, параметр `tableIndex` переписывается в первой строке:
```c
void activate_route_non_blocking(uint8_t tableIndex) {
    tableIndex = tableIndex - 1;  // перезаписывает параметр сразу
```
**Исправление:** Завести явную структуру состояния маршрута (`RouteState`), переписывать параметр в локальную переменную с понятным именем.  
**Статус:** `[ ]`

---

### ARCH-04 — `init_74HC165_ports()` инициализирует 74HC595
**Файл:** `Receiver/scr/shift_registers.c`  
**Описание:** Функция инициализации входного регистра 74HC165 вызывает `shiftOutMultiple` — функцию для выходных регистров 74HC595. Нарушение единственной ответственности.
```c
void init_74HC165_ports(void) {
    DDRD |= (1 << CLOCK_74HC165) | (1 << LATCH_74HC165);
    ...
    shiftOutMultiple(initialData, NUM_OF_74HC595);  // пишет в 595 — не сюда!
}
```
**Исправление:** Перенести обнуление 74HC595 в `init_74HC595_ports()`.  
**Статус:** `[ ]`

---

### ARCH-05 — Взрыв глобального состояния
**Файл:** `Receiver/include/config.h`  
**Описание:** 13 `extern`-переменных объявлены в `config.h` и меняются напрямую из любого файла: `routeSetupInProgress`, `pathMode[]`, `pathSelectedTable[]`, `pathDirection[]`, `emergencyStopActive`, `lastCmd`, `isLocoMoving`, `tickCounter`, `timeCounter`, `sensorStates`, `triggeredBitsHistory`, `SelectedTable`, `previousSensorStates`. Нет единой точки входа для изменения состояния, что делает отладку и тестирование крайне сложными.  
**Исправление:** Сгруппировать связанные переменные в структуры (`PathState`, `LocoState`), скрыть за функциями-аксессорами.  
**Статус:** `[ ]`

---

### ARCH-06 — Блокирующие задержки в главном цикле Receiver и Transmitter
**Файлы:** `Receiver/main.cpp` (`process_packet`), `Transmitter/main.cpp` (`handle_menu_stop`)  
**Описание — Receiver:**
```c
case CMD_STOP:
    send_ack(packet.cmd, packet.param);
    _delay_ms(20);   // блокировка в главном цикле после каждой команды STOP
```
**Описание — Transmitter:**
```c
_delay_ms(600);   // при успешном сохранении в меню
_delay_ms(800);   // при ошибке сохранения в меню
```
600–800 мс блокировки в Transmitter: за это время Receiver может прислать `CMD_ARRIVED` или `CMD_OVER_LOAD_STOP` — они будут потеряны.  
**Исправление:** В Receiver — убрать `_delay_ms(20)` из `process_packet`, либо сделать неблокирующим через тик-счётчик. В Transmitter — заменить задержки на отображение с таймаутом через счётчик итераций.  
**Статус:** `[ ]`

---

## 🟡 КОД-СМЕЛЛ

### SMELL-01 — Пустая функция `LocomotiveSpeedDown()`
**Файл:** `Receiver/scr/PWM.c`  
```c
void LocomotiveSpeedDown(void) {
    // пусто
}
```
Либо реализовать, либо удалить вместе с объявлением в `PWM.h`.  
**Статус:** `[ ]`

---

### SMELL-02 — `UART_receive_packet` продублирован в Receiver и Transmitter
**Файлы:** `Receiver/scr/uart.c`, `Transmitter/uart.c`  
Обе реализации идентичны. Copy-paste. При изменении протокола нужно помнить обновить оба файла.  
**Исправление:** Добавить комментарий о синхронности, или вынести в общий `shared/uart_common.c` если структура проекта позволяет.  
**Статус:** `[ ]`

---

### SMELL-03 — Непоследовательная индексация столов (0-based vs 1-based)
**Файлы:** `loco_control.c`, `railroad_control.c`, `main.cpp`  
Разные функции принимают индекс стола по-разному: одни ожидают 0-based (0..8), другие 1-based (1..9). Об этом говорят многочисленные `tableIndex - 1` и `tableIndex + 1` по всему коду.  
**Исправление:** Принять единое соглашение (предпочтительно 1-based как у пользователя), зафиксировать в `config.h` и `CLAUDE.md`, конвертировать только на границе с железом.  
**Статус:** `[ ]`

---

### SMELL-04 — Закомментированный мёртвый код
**Файлы:** `Receiver/scr/adcRead.c`, `Receiver/main.cpp`, `Receiver/scr/loco_control.c`  
- `show_adc_value_on_lcd()` — вызов закомментирован в `update_adc_display_if_due()`
- `run_output_shift_register_test()` — закомментирован в `main()`
- `_delay_ms(150)` и `_delay_ms(80)` — закомментированы в `MoveLocoBackward()`

**Исправление:** Удалить или обернуть в `#ifdef DEBUG`.  
**Статус:** `[ ]`

---

## 🔵 ДОПОЛНИТЕЛЬНЫЕ УЛУЧШЕНИЯ

### ADD-01 — Проверка флага DOR (UART overrun) в Receiver
**Файл:** `Receiver/scr/uart.c`  
Добавить проверку `UCSR0A & (1 << DOR0)` в `UART_receive()`. При overrun — сброс и возврат ошибки вместо попытки собрать пакет из повреждённых данных.  
**Статус:** `[ ]`

---

### ADD-02 — Включить Watchdog Timer (оба МК)
При зависании прошивки автоматический сброс вместо ручного отключения питания. Период ~250 мс, `wdt_reset()` в главном цикле.  
**Статус:** `[ ]`

---

### ADD-03 — `_delay_ms(80)` в `MoveLocoBackward` → неблокирующий
**Файл:** `Receiver/scr/loco_control.c`  
Последняя блокирующая задержка в главном пути — сделать через `rail_switch_step_counter`.  
**Статус:** `[ ]`

---

### ADD-04 — `_delay_ms(50)` в диагностике overload → неблокирующий
**Файл:** `Receiver/scr/protection.c`, `transmit_diagnostic_results()`  
50 мс между двумя `send_command(CMD_DIAG_RESULT, ...)` — заменить на тик-счётчик.  
**Статус:** `[ ]`

---

### ADD-05 — Версионирование прошивки
Добавить `FW_VERSION_MAJOR/MINOR` в `config.h`, отображать на LCD при старте и в сервисном меню.  
**Статус:** `[ ]`

---

### ADD-06 — Перевести русские комментарии на английский
Кодировка CP1251 даёт кракозябры в UTF-8 редакторах. Перевести всё на английский, файлы сохранить в UTF-8.  
**Статус:** `[ ]`

---

## Приоритизация

| ID | Задача | Сложность | Риск | Приоритет |
|----|--------|-----------|------|-----------|
| BUG-01 | PWM.c убивает PATH2 | Малая | Низкий | 🔴 Высокий |
| BUG-04 | ~~Мёртвая проверка сенсоров~~ | Малая | Нет | ✅ Готово |
| ADD-01 | Проверка DOR UART | Малая | Низкий | 🔴 Высокий |
| ADD-02 | Watchdog Timer | Малая | Средний | 🔴 Высокий |
| BUG-02 | ~~CMD_SLOW_MODE / CMD_IDEL_STOP~~ | Малая | Низкий | ✅ Готово |
| ARCH-06 | Задержки в главном цикле | Средняя | Средний | 🟡 Средний |
| ADD-03 | 80ms delay → неблокирующий | Средняя | Низкий | 🟡 Средний |
| ARCH-01 | Дубликат get_table_path | Малая | Низкий | 🟡 Средний |
| ARCH-02 | (void)data bug | Средняя | Высокий | 🟡 Средний |
| SMELL-03 | Единая индексация столов | Большая | Высокий | 🟢 Низкий |
| ARCH-05 | Глобальное состояние | Большая | Высокий | 🟢 Низкий |
| SMELL-01 | Пустая LocomotiveSpeedDown | Малая | Нет | 🟢 Низкий |
| SMELL-04 | Мёртвый код | Малая | Нет | 🟢 Низкий |
| ADD-05 | Версионирование | Малая | Нет | 🟢 Низкий |
| ADD-06 | Перевод комментариев | Большая | Нет | 🟢 Низкий |
