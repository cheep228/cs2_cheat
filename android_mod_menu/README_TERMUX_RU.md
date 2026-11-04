# Запуск с телефона (Termux) — без ПК, без Python

Самый простой путь для «только телефон»: готовый бинарь **`frida-inject`**
грузит скрипт прямо в процесс игры. Не нужны ни Python, ни frida-server, ни adb.
Termux нужен только как root-терминал. Root у тебя есть (SukiSU). ✅

Пакет игры: `com.JamesWest.NoomiClone`

---

## 0. Поставь Termux (один раз)
Ставь **с F-Droid или GitHub** (версия из Play Market старая и не годится):
https://github.com/termux/termux-app/releases

## 1. Пакеты в Termux
```bash
pkg update -y
pkg install -y curl xz-utils
termux-setup-storage        # даст доступ к /sdcard (разреши запрос)
```

## 2. Положи файлы в Termux
Файлы `menu.js`, `dump.js`, `get_frida_inject.sh` ты скачал из чата — они обычно
в `/sdcard/Download`. Перенеси их в домашнюю папку Termux:
```bash
cd ~
cp /sdcard/Download/menu.js /sdcard/Download/dump.js /sdcard/Download/get_frida_inject.sh ~/
```
(если скачалось в другое место — поправь путь)

## 3. Скачай frida-inject
```bash
bash get_frida_inject.sh 17.25.2
```
Появится файл `~/frida-inject`.

## 4. Скопируй всё в /data/local/tmp (оттуда можно исполнять)
```bash
su -c 'cp ~/frida-inject ~/menu.js ~/dump.js /data/local/tmp/ && chmod 755 /data/local/tmp/frida-inject'
```

## 5. Запуск меню
Сначала **запусти игру руками**, зайди в геймплей. Потом в Termux:
```bash
su -c '/data/local/tmp/frida-inject -n com.JamesWest.NoomiClone -s /data/local/tmp/menu.js -e'
```
Через пару секунд в углу появится **🎃**. Тыкни — откроется меню. Тыкву можно таскать.

- Флаг `-e` = «eternalize»: скрипт останется жить, даже если закроешь Termux.
- Если по имени не цепляется — по PID:
  ```bash
  su -c 'P=$(pidof com.JamesWest.NoomiClone); /data/local/tmp/frida-inject -p $P -s /data/local/tmp/menu.js -e'
  ```

## 6. Разведка полей (нужно мне для точной настройки)
```bash
su -c '/data/local/tmp/frida-inject -n com.JamesWest.NoomiClone -s /data/local/tmp/dump.js'
```
В терминале напечатаются кандидаты (деньги/перехваты). Полный дамп ляжет в файл —
вытащи его на /sdcard и пришли мне:
```bash
su -c 'cp /data/data/com.JamesWest.NoomiClone/noomi_dump.cs /sdcard/noomi_dump.cs'
```
Файл будет в `/sdcard/noomi_dump.cs` — скинь его в чат.

---

## Если игра закрывается при запуске frida (NXGATE детектит)
По нарастающей:
1. Переименуй бинарь, чтобы не палиться по имени:
   ```bash
   su -c 'cp /data/local/tmp/frida-inject /data/local/tmp/fj && chmod 755 /data/local/tmp/fj'
   su -c '/data/local/tmp/fj -n com.JamesWest.NoomiClone -s /data/local/tmp/menu.js -e'
   ```
2. Включи в SukiSU **hide/скрытие рута** для процесса игры.
3. Крайний случай — frida-gadget через LSPatch (это уже патч апк).

Напиши, что именно происходит (краш сразу / через N секунд / чёрный экран) — подскажу точнее.
