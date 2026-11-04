# Переезд на ksurusda (анти-детект gadget) — чтобы NXGATE нас не видел

Почему: KsuFrida инжектит, но NXGATE нас палит (в логе `blocked killProcess`/
`System.exit`) → игра не проходит свой стартовый гейт → чёрный экран. Убрать
античит нельзя — он расшифровывает код игры (метаданные скремблены). Значит
надо спрятаться. ksurusda это умеет из коробки: прячет имена потоков frida,
строки `frida:rpc`/`memfd:frida-agent` и **ремаппит память, убирая наш .so из
`/proc/self/maps`** (самый вероятный вектор детекта).

Формат файлов тот же, что у KsuFrida (`/data/local/tmp/libsec/`), переезд простой.

---

## 1. Убери старый модуль KsuFrida
Чтобы два модуля не инжектили разом: в SukiSU → Модули → **удали KsuFrida** → перезагрузка.

## 2. Поставь ksurusda
Скачай браузером:
```
https://github.com/zensu357/ksurusda/releases/download/v2.0.2/KsuRusda-v2.0.2.zip
```
SukiSU → Модули → установить из файла → `KsuRusda-v2.0.2.zip` → **перезагрузка**.
(Zygisk должен быть включён — как и раньше.)

## 3. Проверь, что модуль разложил свой gadget
```
su -c 'ls -la /data/local/tmp/libsec/'
```
Должны быть **его** `libsecmon.so` (и, возможно, `libsecmon32.so`). Это анти-детект
gadget — его НЕ трогаем и НЕ перезаписываем.

## 4. Положи наши 3 файла (НЕ трогая libsecmon.so)
Нужны из Загрузок: `config.json`, `libsecmon.config.so`, `script.js`
(это наш menu, переименованный под ksurusda). Одной пачкой:
```
su -c 'cp "$(ls -t /sdcard/Download/config.json | head -1)" /data/local/tmp/libsec/config.json'
su -c 'cp "$(ls -t /sdcard/Download/libsecmon.config.so | head -1)" /data/local/tmp/libsec/libsecmon.config.so'
su -c 'cp "$(ls -t /sdcard/Download/script*.js | head -1)" /data/local/tmp/libsec/script.js'
su -c 'chmod 755 /data/local/tmp/libsec; chmod 644 /data/local/tmp/libsec/config.json /data/local/tmp/libsec/libsecmon.config.so /data/local/tmp/libsec/script.js'
su -c 'rm -f /data/data/com.JamesWest.NoomiClone/.cache/menu.log'
```

Проверь, что всё на месте (должно быть 4+ файла, включая их `libsecmon.so`):
```
su -c 'ls -la /data/local/tmp/libsec/'
```

## 5. (по желанию) включи задержку старта
Если сразу не взлетит — помогает дать gadget'у отработать до NXGATE. В `config.json`
уже стоит `"start_up_delay_ms": 0`; можно поднять до `3000`:
```
su -c "sed -i 's/\"start_up_delay_ms\": 0/\"start_up_delay_ms\": 3000/' /data/local/tmp/libsec/config.json"
```

## 6. Запусти игру, подожди ~15 сек
Смотри: загрузилась дальше чёрного экрана? появилась 🎃?

## 7. Лог
```
su -c 'cat /data/data/com.JamesWest.NoomiClone/.cache/menu.log'
```
- Если NXGATE нас больше не видит — **НЕ будет** `blocked killProcess`/`System.exit`,
  игра загрузится, появится 🎃.
- Если `blocked …` снова есть — детект всё же срабатывает; тогда поднимем
  `start_up_delay_ms`, включим в ksurusda доп.опции, либо разберём конкретный чек.

Пришли вывод лога + что на экране.
