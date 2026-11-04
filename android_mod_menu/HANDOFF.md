# HANDOFF — NoomiCloneX mod menu (для Claude Code на устройстве)

Это передача задачи Claude Code, запущенному **на самом Android-устройстве** (Termux + root).
У тебя, в отличие от облачной сессии, есть ПРЯМОЙ доступ: можешь сам запускать frida,
читать логи, править конфиги и перезапускать игру. Используй это — итерируй на устройстве.

## Цель
Плавающее мод-меню 🎃 в офлайн-игре **NoomiCloneX** (`com.JamesWest.NoomiClone`, Unity 6 IL2CPP, arm64):
изменение денег, «перехватов», режим Fly. Устройство рутовано (SukiSU/KernelSU), владелец — игрок, игра офлайн, одиночная.

## Что за игра / защита (уже выяснено)
- Unity **6000.6.0f1**, **IL2CPP**, только `arm64-v8a`. `libil2cpp.so` экспортирует все `il2cpp_*` → frida-il2cpp-bridge работает в рантайме.
- Защита **NXGATE**: `lib/arm64-v8a/libge32c849974168bd8.so` + `assets/g5924ea53771c0357.bin` (магия `NXGATE02R`).
- `global-metadata.dat` имеет **скремблённый заголовок** (version=108 вместо 31) → NXGATE расшифровывает метаданные в рантайме ⇒ **его НЕЛЬЗЯ вырезать** (игра не запустится). Нужно его обойти, а не удалять.
- Пакет: `com.JamesWest.NoomiClone`, launcher `com.noomiclonex.MrxGameActivity`, v1.7.

## Что уже сделано (в этой папке `android_mod_menu/`)
- `src/menu.ts` → компилируется `frida-compile` в `dist/menu.js` (= `zygisk/script.js`). Это полезная нагрузка gadget'а (script-режим).
  Содержит:
  - **UI меню** (Java-мост `frida-java-bridge`, overlay в decorView активити): деньги/перехваты/fly/поиск поля/dump.
  - **anti-kill преамбула**: хуки `exit/_exit/abort/kill/tgkill/pthread_kill/raise` (только смертельные сигналы) + Java `System.exit`/`Runtime.*`/`Process.killProcess(self)`.
  - **proc-evasion**: hook `open/openat` → подмена `/proc/self/maps` на «чистую» копию (без строк `libsecmon/frida/gum/.cache`); нативное переименование потоков frida в `comm`.
  - лог-маячок пишется в `/data/data/com.JamesWest.NoomiClone/.cache/menu.log` (ф-ция `akLog`).
- `zygisk/config.json` — таргет KsuFrida/ksurusda (формат `targets`, injected_libraries → `/data/local/tmp/libsec/libsecmon.so`).
- `zygisk/libsecmon.config.so` — конфиг gadget в **script-режиме** → `/data/local/tmp/libsec/script.js`.
- `CONFIG` в `menu.ts` для денег/перехватов **пустой** → работает эвристика (меняет числовые поля по именам money/coin/grip/...). Нужен дамп, чтобы прицелиться точно.

## Пересборка (нужны node+npm)
```
cd android_mod_menu
npm install            # frida-compile, frida-il2cpp-bridge, frida-java-bridge, @types/frida-gum
npm run build          # -> dist/dump.js, dist/menu.js
cp dist/menu.js zygisk/script.js
```

## Текущий рабочий стенд на устройстве
- `/data/local/tmp/libsec/`: `libsecmon.so` (frida-gadget 17.25.2 arm64, 39 МБ), `libsecmon.config.so` (script→script.js), `config.json`, `script.js`.
- Инжектор: **KsuFrida** (gorkemgun/ksu-frida, модуль Zygisk). Он НАДЁЖНО впрыскивает (в логах `KsuFrida: Injected libsecmon.so`).
- **ksurusda** (анти-детект модуль) на этом устройстве НЕ инжектит вообще — не трать на него время.
- Прочие модули: Zygisk Next, Vector (Xposed), Shamiko, tricky_store, LuckyPatcher. Глобальный «umount modules» был выключен для игры.

## ГДЕ МЫ ЗАСТРЯЛИ (главное)
Инъекция работает, но **NXGATE детектит frida и убивает игру**:
- Под KsuFrida в `menu.log` ранее было: `preamble installed OK`, затем `blocked killProcess self`, `blocked System.exit 0` — т.е. NXGATE детектил и дёргал Java-выход; мы его гасили → процесс жив, но «стартовый гейт» не проходит → **чёрный экран**.
- Детект происходит в нативном `libge32...so`, который грузится ~через 0.9с после инъекции (gadget встаёт раньше — успеваем ставить хуки ДО него).
- Что уже пробовали против детекта: anti-kill (переживает, но чёрный экран), переименование потоков (не помогло), подмена `/proc/self/maps` (последняя версия, результат на устройстве не подтверждён).

## ПЛАН ДЕЙСТВИЙ ДЛЯ ТЕБЯ (на устройстве)
1. Убедись, что инъекция идёт и НАШ скрипт грузится: запусти игру, проверь `/data/data/com.JamesWest.NoomiClone/.cache/menu.log`. Должно быть `preamble installed OK` + `proc evasion installed`.
   - Если лога нет → gadget не в script-режиме. Проверь `libsecmon.config.so` рядом со staged-gadget (KsuFrida копирует в `.../.cache/libsecmon.so`, gadget ищет `.cache/libsecmon.config.so`). Возможно, надо класть наш config в `.cache` или прописать абсолютный путь скрипта.
2. **Найди реальный вектор детекта** (ключ к успеху). Поскольку у тебя прямой доступ — сними полный лог краша и, главное, **хукни сам NXGATE**:
   - под frida пройдись Interceptor'ом по экспортам/функциям `libge32c849974168bd8.so`, лог-трейсни `open/openat/read/fopen/access/stat`, `dl_iterate_phdr`, `pthread_create`, `readlink`, `/proc/self/*`, `strstr/strcmp` с аргументами — увидишь, что именно он проверяет (имя .so в maps? строки в памяти? порт? ptrace self? /proc/self/status TracerPid?).
   - по найденному чеку сделай точечный обход (подмена возвращаемых данных / чтений).
3. Fly сейчас = анти-гравитация (обнуление `Physics.gravity`). После того как увидишь контроллер игрока в дампе — сделай нормальный направленный полёт.
4. Деньги/перехваты: запусти `dist/dump.js` (или кнопку Dump в меню) → получишь `/data/data/.../noomi_dump.cs`; найди поля money/coin/grip; впиши в `CONFIG` в `menu.ts`, пересобери.

## Полезное
- Репозиторий: `cheep228/cs2_cheat`, ветка `ccr-7da78d9f-dtgbln`. Коммить сюда прогресс.
- Frida 17 убрал глобальный `Java` → он импортируется из `frida-java-bridge` (уже в коде).
- APK игры при необходимости: Google Drive id `1ZDYULJ5ETAC2E0_3SgHZsBZAtVvjysPO`.
- Эвристика денег может задевать лишние поля — логируй каждое изменение, чтобы владелец сказал, какое сработало.

Удачи. У тебя есть то, чего не было у облачной сессии — живое устройство. Трейсни NXGATE и добей.
