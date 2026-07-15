# EPT - Extended ProtoThreads

EPT - это самостоятельная реализация кооперативных потоков, выросшая из классических [протопотоков](https://dunkels.com/adam/pt/index.html) Адама Данкелса. От исходных pt осталась только идея local continuations на `switch` (`lc.h`); всё ядро живёт в одном заголовке `ept.h`. Никаких аллокаций, никаких приоритетов, никакого вытеснения - потоки представляют собой обычные функции, которые вызываются планировщиком по кругу.

---

# Запуск минимального проекта

Ядро не зависит от платформы: нужен любой MCU (или хост), у которого есть периодическое прерывание. Период не обязан быть миллисекундным - частота задаётся через `EPT_TICK_FREQ_HZ` (например, 200 Гц = 5 мс, 100 Гц = 10 мс); период тика становится квантом времени для `EPT_SLEEP` и таймаутов. Ограничение: 1000 должно делиться на частоту нацело (`EPT_TICK_PERIOD_MS = 1000 / EPT_TICK_FREQ_HZ` - целочисленное деление, частоты выше 1000 Гц дают нулевой период). Ниже - минимальный набор шагов «от нуля до мигающего светодиода».

## Шаг 1. Добавить файлы

Подключите репозиторий как git submodule (или просто скопируйте файлы):

```
git submodule add https://github.com/j3qq4hch/EPT.git libs/EPT
```

Минимальному проекту из всего репозитория нужны:

| Файл | Что это |
|---|---|
| `ept.h`, `lc.h`, `lc-switch.h` | ядро (header-only) |
| `ept_scheduler.c`, `ept_scheduler.h` | планировщик - единственный компилируемый файл |

Остальное (CLI, settings, uart_pt, drv_basic, log, to_string) - опциональные модули, см. таблицу в конце.

В проект добавьте **только `ept_scheduler.c`**; путь к `libs/EPT` - в include paths.

## Шаг 2. Задать частоту тика

В настройках препроцессора (не в исходниках!) задайте:

```
EPT_TICK_FREQ_HZ=1000
```

При 1000 Гц можно не задавать - это значение по умолчанию (компилятор напомнит через `#pragma message`). Важно: определять её в `ept_cfg.h` бесполезно - `ept.h` включается планировщиком раньше, чем `ept_cfg.h`.

## Шаг 3. Создать ept_cfg.h

Это конвенционный файл проекта (шаблон - `ept_cfg_template.h`), он должен лежать в include paths проекта:

```c
#ifndef EPT_CFG_H_
#define EPT_CFG_H_
#endif

// ---- Thread list ----
#ifdef EPT_SCHEDULER_IMPLEMENTATION

EPT_THREAD(blinker(struct ept *ept)); // forward-декларации всех потоков верхнего уровня

static const thread_record threadlist[] = {
//   name      thread   context  init_state  init_dbg_level
    {"blink",  blinker, NULL,    RUN,        0},
};

#endif // EPT_SCHEDULER_IMPLEMENTATION
```

## Шаг 4. Тик из SysTick

В обработчике системного таймера (частота = `EPT_TICK_FREQ_HZ`):

```c
#include "ept_scheduler.h"

void SysTick_Handler(void)
{
    ept_scheduler_tick();
}
```

## Шаг 5. Первый поток и main

```c
#include "ept_scheduler.h"

EPT_THREAD(blinker(struct ept *ept))
{
    EPT_BEGIN(ept);
    while (1)
    {
        EPT_SLEEP(ept, 500);
        led_toggle(); // ваш код
    }
    EPT_END(ept);
}

int main(void)
{
    hw_init();       // клоки, GPIO, SysTick на EPT_TICK_FREQ_HZ
    ept_scheduler(); // управление не возвращается
}
```

Собираете, прошиваете - светодиод мигает. Это весь минимальный проект: два своих файла (`ept_cfg.h` и `main.c`) плюс `ept_scheduler.c` из библиотеки.

## Шаг 6 (опционально). Энергосбережение

`#define LOW_POWER_MODE` в `ept_cfg.h` заставляет планировщик звать `drv_sleep()` (WFI), когда ни один поток не вернул `EPT_ACTIVE`. `drv_sleep()` приезжает из `drv_basic.h` - для STM32C0/F0 порты готовы (требуют LL-заголовков ST в include paths), для другой платформы напишите свой `drv_basic_<mcu>.h` c одной строчкой `static inline void drv_sleep(void) { __WFI(); }`. Для первого запуска проще LOW_POWER_MODE не включать.

Любое прерывание, включая SysTick, будит MCU, поэтому сон ограничен одним тиком и опрашиваемые условия не могут быть пропущены. Проверить, что устройство реально спит, можно CLI-командой `loops` (см. ниже): в простое она показывает ~`EPT_TICK_FREQ_HZ` проходов в секунду; сотни тысяч означают, что кто-то зря возвращает `EPT_ACTIVE`.

---

# Справочник

## Статусы потоков

Каждый вызов потока возвращает один из трёх статусов:

| Статус | Значение |
|---|---|
| `EPT_ACTIVE` | у потока есть работа прямо сейчас - засыпать нельзя |
| `EPT_BLOCKED` | поток ждёт условия или таймера - MCU может спать до следующего тика/прерывания |
| `EPT_DONE` | поток завершился (имеет смысл для дочерних потоков) |

Инвариант: `EPT_ALIVE(s) == (s < EPT_DONE)`. `EPT_SPAWN` пробрасывает статус дочернего потока родителю без изменений, поэтому спящий на любом уровне вложенности поток не мешает переходу MCU в энергосберегающий режим.

## Основные макросы

- `EPT_THREAD(name(struct ept *ept))` / `EPT_BEGIN(ept)` / `EPT_END(ept)` - определение потока;
- `EPT_WAIT_UNTIL / EPT_WAIT_WHILE` - блокирующее ожидание условия;
- `EPT_WAIT_UNTIL_TIMEOUT` - ожидание условия с таймаутом;
- `EPT_SLEEP(ept, time_ms)` - сон на встроенном таймере потока;
- `EPT_YIELD` - отдать один проход планировщика, продолжить;
- `EPT_YIELD_UNTIL / EPT_YIELD_WHILE` - уступить минимум один раз, затем ждать условия;
- `EPT_SPAWN / EPT_WAIT_THREAD` - запуск дочернего потока с пропагацией статуса;
- `EPT_RESTART / EPT_EXIT` - перезапуск / досрочное завершение.

Ограничения switch-based LC: внутри тела потока нельзя использовать `switch` через точку блокировки, два блокирующих макроса не должны находиться на одной строке, локальные переменные не переживают блокировку (используйте `static` или контекст потока `thread_ctx`).

## drv_basic

`drv_basic` - это минимально необходимый набор штук, без которых никакая прошивка не имеет смысла:

- **GPIO** (`pin_t`: set/reset/toggle/read),
- **UART** (`uart_t`: неблокирующий TX по DMA, RX через кольцевой буфер в ISR),
- **flash** (`flash_page_erase` / `flash_write_data` - бэкенд для хранения настроек),
- **сон** (`drv_sleep()` для LOW_POWER_MODE).

Ядру EPT он не нужен - но любая реальная прошивка начинается именно с этого, поэтому набор живёт рядом с ядром. `drv_basic.h` - диспетчер, выбирающий порт по define семейства (`STM32C011xx`, `STM32F0xx`); готовые порты в репозитории: `drv_basic_stm32c0xx.h`, `drv_basic_stm32f0xx.h` (во втором пока нет flash-бэкенда).

Для другой платформы порт легко соорудить руками: это один заголовок в STB-стиле, где нужно реализовать только то, чем реально пользуетесь - для мигалки достаточно `pin_t` с четырьмя однострочными inline-функциями, CLI добавляет UART-набор, settings - две flash-функции, LOW_POWER_MODE - однострочный `drv_sleep()`. Возьмите любой из готовых портов как образец сигнатур.

## Опциональные модули

| Модуль | Даёт | Требует |
|---|---|---|
| `drv_basic.h` + `drv_basic_stm32*.h` | GPIO/UART(DMA)/flash/`drv_sleep()` | LL-заголовки ST; `#define DRV_BASIC_IMPLEMENTATION` в одном TU |
| `uart_pt.h` | неблокирующие UART-операции для потоков | drv_basic |
| `ept_cli.c/.h` | CLI через UART (команды `h`, `ept`, `dbg`, `loops`, ...) | uart_pt, файл `cli_cmd_map.h` (шаблон прилагается), настройки CLI_* в `ept_cfg.h` |
| `ept_settings.c/.h` | настройки в flash с CRC и CLI-командой `set` | файл `settings_cfg.h`, flash-опы из drv_basic |
| `log.h` | отладочный вывод с уровнями на поток | `DEBUG_PRINT()` в `ept_cfg.h` |
| `to_string.h` | `to_string()` через `_Generic`, расширяемый проектными типами | файл `resources.h` с `EPT_TYPES_LIST` |
| `PROFILER` (в `ept_cfg.h`) | максимальное время исполнения каждого потока, команда `prof` | CMSIS-заголовок в `ept_cfg.h`, `EPT_CPU_FREQ_HZ` в препроцессоре |

Конвенция: проект предоставляет до четырёх своих заголовков - `ept_cfg.h` (всегда), `cli_cmd_map.h` (если CLI), `settings_cfg.h` (если settings), `resources.h` (если to_string).

## Тесты

`tests/torture_test.c` - хостовый (без железа) прогон ядра через виртуальное время: sleep/spawn/yield/timeout/restart, вложенные потоки, переполнение счётчика тиков, пропагация статусов.

```
gcc -Wall -Wextra -Werror -Wno-implicit-fallthrough -o torture torture_test.c && ./torture
```
