#!/usr/bin/env bash
# Качает готовый бинарь frida-inject для Android arm64.
# Запускать В TERMUX на телефоне. Python НЕ нужен.
set -e
FRIDA_VERSION="${1:-17.25.2}"
NAME="frida-inject-${FRIDA_VERSION}-android-arm64"
URL="https://github.com/frida/frida/releases/download/${FRIDA_VERSION}/${NAME}.xz"

echo "[*] Качаю $URL"
curl -L -o "${NAME}.xz" "$URL"
echo "[*] Распаковываю"
xz -d -f "${NAME}.xz"
mv -f "${NAME}" frida-inject
chmod +x frida-inject
echo "[+] Готово: $(pwd)/frida-inject"
echo
echo "Дальше — положи рядом menu.js (и dump.js) и запускай от root (см. README_TERMUX_RU.md)"
