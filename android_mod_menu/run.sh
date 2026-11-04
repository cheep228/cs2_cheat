#!/usr/bin/env bash
# Запуск мод-меню. Нужен запущенный frida-server на телефоне (от root)
# и frida-tools на этой машине (ПК по USB или сам телефон через Termux).
set -e
PKG="com.JamesWest.NoomiClone"
DIR="$(cd "$(dirname "$0")" && pwd)"
MODE="${1:-attach}"   # attach | spawn | dump

case "$MODE" in
  spawn)
    # запустить игру под управлением frida (меню с самого старта).
    # минус: анти-тампер NXGATE может ругаться на ранний старт.
    frida -U -f "$PKG" -l "$DIR/dist/menu.js"
    ;;
  attach)
    # сначала руками запусти игру, дойди до экрана/геймплея, потом это:
    frida -U -n "$PKG" -l "$DIR/dist/menu.js"
    # если по имени не цепляется, попробуй: frida -U Gadget ... или -F (foreground)
    ;;
  dump)
    # разведка полей (деньги/перехваты). Игра должна быть запущена.
    frida -U -n "$PKG" -l "$DIR/dist/dump.js"
    ;;
  *)
    echo "usage: $0 [attach|spawn|dump]"; exit 1;;
esac
