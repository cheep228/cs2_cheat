#!/system/bin/sh
# Шаг 3: раскладывает наши файлы для KsuFrida в /data/local/tmp/libsec/
# Запуск (из Termux):  su -c 'sh /sdcard/Download/setup_libsec.sh'
# Файлы (config.json, libsecmon.config.so, menu.js) должны лежать в Загрузках.

DEST=/data/local/tmp/libsec

# где искать исходники
CANDS="/sdcard/Download /sdcard/Downloads /storage/emulated/0/Download /storage/emulated/0/Downloads /sdcard /data/data/com.termux/files/home/storage/downloads /data/data/com.termux/files/home /data/local/tmp"

find_file() {
  name="$1"
  for d in $CANDS; do
    if [ -f "$d/$name" ]; then echo "$d/$name"; return 0; fi
  done
  return 1
}

echo "=== создаю $DEST ==="
mkdir -p "$DEST"

ok=1
for f in config.json libsecmon.config.so menu.js; do
  src=`find_file "$f"`
  if [ -z "$src" ]; then
    echo "[!] НЕ НАЙДЕН: $f  — положи его в Загрузки (Download)"
    ok=0
    continue
  fi
  cp "$src" "$DEST/$f" && echo "[+] $f   <-  $src"
done

chmod 755 "$DEST" 2>/dev/null
chmod 644 "$DEST/config.json" "$DEST/libsecmon.config.so" "$DEST/menu.js" 2>/dev/null

echo
echo "=== содержимое $DEST ==="
ls -la "$DEST"

echo
if [ -f "$DEST/libsecmon.so" ]; then
  echo "[OK] libsecmon.so на месте (модуль KsuFrida установлен)."
else
  echo "[ВНИМАНИЕ] libsecmon.so НЕТ — значит модуль KsuFrida ещё не установлен"
  echo "           или не было перезагрузки. Поставь zip и перезагрузись (шаги 0-1)."
fi

if [ "$ok" = "1" ]; then
  echo
  echo "[ГОТОВО] Теперь просто запусти игру — через пару секунд должна появиться тыква."
fi
