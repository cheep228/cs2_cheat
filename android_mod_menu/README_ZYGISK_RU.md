# Обход NXGATE через Zygisk (KsuFrida) + наш menu.js

Грузим наш `menu.js` в игру через **Zygisk-инъекцию без ptrace** — frida-gadget
попадает в процесс на старте, до того как защита поднимется. Gadget работает в
**script-режиме**: сам запускает `menu.js`, НЕ открывает порт, НЕ нужен клиент.
Это убирает сразу три вектора детекта (ptrace, порт 27042, явное имя «frida»).

Пакет игры: `com.JamesWest.NoomiClone`

---

## 0. Включи Zygisk в SukiSU (один раз)
KsuFrida требует Zygisk. В SukiSU (Ultra) включи Zygisk в настройках. Если
переключателя нет — поставь модуль **Zygisk Next** и перезагрузись.

## 1. Поставь модуль KsuFrida
Скачай релиз-zip (браузером телефона):
```
https://github.com/gorkemgun/ksu-frida/releases/download/v1.9.46/KsuFrida-v1.9.46.zip
```
Открой SukiSU → **Модули** → установить из файла → выбери `KsuFrida-v1.9.46.zip` → **перезагрузка**.

После ребута модуль сам создаст папку `/data/local/tmp/libsec/` и положит туда
свой gadget `libsecmon.so`. Проверь:
```
su -c 'ls -la /data/local/tmp/libsec/'
```
Должен быть как минимум `libsecmon.so` (и, возможно, `config.json`, `libsecmon.config.so`).

## 2. Положи наши файлы
Тебе нужно закинуть в `/data/local/tmp/libsec/` три вещи:
`config.json`, `libsecmon.config.so` (оба я прислал в чат) и `menu.js` (уже есть у тебя).

Если файлы из чата лежат в `~/storage/downloads` (Termux) — в Termux:
```
su -c 'mkdir -p /data/local/tmp/libsec'
su -c "cp ~/storage/downloads/config.json /data/local/tmp/libsec/config.json"
su -c "cp ~/storage/downloads/libsecmon.config.so /data/local/tmp/libsec/libsecmon.config.so"
su -c "cp /data/data/com.termux/files/home/menu.js /data/local/tmp/libsec/menu.js"
```
(если `menu.js` у тебя в Download, а не в домашней Termux — поправь путь на
`~/storage/downloads/menu.js`)

Дай права на чтение (gadget читает их уже от имени игры):
```
su -c 'chmod 755 /data/local/tmp/libsec; chmod 644 /data/local/tmp/libsec/config.json /data/local/tmp/libsec/libsecmon.config.so /data/local/tmp/libsec/menu.js'
```

Проверь, что всё на месте:
```
su -c 'ls -la /data/local/tmp/libsec/'
```
Должны быть: `config.json`, `libsecmon.so`, `libsecmon.config.so`, `menu.js`.

> Важно: `config.json` говорит модулю грузить gadget в нашу игру.
> `libsecmon.config.so` переводит gadget в script-режим на `menu.js`.
> Имена файлов менять нельзя — gadget ищет конфиг строго как `libsecmon.config.so`
> рядом с `libsecmon.so`.

## 3. Запусти игру
Просто открой игру как обычно (НИКАКОГО Termux/frida руками — всё само).
Через пару секунд после входа в геймплей должна появиться **🎃**.
Тыкай — меню (Деньги / Перехваты / Fly).

## 4. Если не видно логов / не работает — смотрим logcat
```
su -c 'logcat -c'        # очистить, потом запусти игру, потом:
su -c 'logcat -d' | grep -iE "frida|gadget|il2cpp|menu|libsec|noomi|FATAL|SIGSEGV|nxgate" | tail -80
```
Строки `il2cpp готов`, `Меню-тыква прикреплено` = наш скрипт жив.
Скинь этот вывод мне — пойму, на каком этапе.

---

## Если игра всё равно крашится (NXGATE ловит сам gadget)

Zygisk убрал ptrace/порт/имя, но у стокового frida-gadget внутри остаются строки
`frida`/`gum` и потоки `gum-js-loop`, `gmain` — защита может сканить их в
`/proc/self/task/*/comm` и памяти. Тогда эскалация:

1. **Задержка старта.** В `config.json` поставь `"start_up_delay_ms": 4000` —
   иногда помогает проскочить ранний чек. Скопируй заново и перезапусти игру.
2. **Патченый (анти-детект) gadget.** Заменим `libsecmon.so` на gadget с
   переименованными строками/потоками (strongR-frida / Rusda). Я подскажу точную
   сборку — ты просто положишь её вместо `libsecmon.so` (имя оставить `libsecmon.so`).
3. **child_gating** если игра форкает защитный процесс — включим.

Напиши, что именно в логе (имя детекта, сигнал), и я дам следующий шаг.

---

## Файлы этого этапа
- `zygisk/config.json` — цель инъекции (наша игра).
- `zygisk/libsecmon.config.so` — gadget в script-режиме → `menu.js`.
- `dist/menu.js` — само меню (я уже присылал).
