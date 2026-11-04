#!/usr/bin/env bash
# Скачивает frida-server для Android arm64 нужной версии.
# Версия ДОЛЖНА совпадать с установленным frida-tools (major.minor).
# Запускать на ПК или в Termux (там, где есть интернет без ограничений).
set -e
FRIDA_VERSION="${1:-17.25.2}"
ARCH="android-arm64"
NAME="frida-server-${FRIDA_VERSION}-${ARCH}"
URL="https://github.com/frida/frida/releases/download/${FRIDA_VERSION}/${NAME}.xz"

echo "[*] Качаю $URL"
curl -L -o "${NAME}.xz" "$URL"
echo "[*] Распаковываю"
xz -d -f "${NAME}.xz"
mv -f "${NAME}" frida-server
chmod +x frida-server
echo "[+] Готово: ./frida-server  (frida-server ${FRIDA_VERSION})"
echo
echo "Дальше (с ПК по USB, нужен adb и root на телефоне):"
echo "  adb push frida-server /data/local/tmp/fs"
echo "  adb shell \"su -c 'chmod 755 /data/local/tmp/fs; /data/local/tmp/fs &'\""
echo
echo "Или прямо на телефоне (Termux + root):"
echo "  cp frida-server \$HOME/fs && su -c 'cp '\$HOME'/fs /data/local/tmp/fs; chmod 755 /data/local/tmp/fs; /data/local/tmp/fs &'"
