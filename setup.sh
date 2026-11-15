#!/bin/sh
# setup.sh - Install Dots and Boxes on Alpine Linux so it starts on boot with no login.
#
#   sh setup.sh            install (build, copy binary, replace tty1 login with the game)
#   sh setup.sh uninstall  restore the normal login prompt on tty1
#
# Run as root from inside the project folder, then reboot.

set -e

BIN=/usr/local/bin/dots_and_boxes
INITTAB=/etc/inittab
BACKUP=/etc/inittab.before-dots

if [ "$(id -u)" -ne 0 ]; then
    echo "Please run this script as root."
    exit 1
fi

if [ "$1" = "uninstall" ]; then
    if [ -f "$BACKUP" ]; then
        cp "$BACKUP" "$INITTAB"
        echo "Restored the original $INITTAB. Reboot to get the login prompt back on tty1."
    else
        sed -i "s|^tty1::respawn:$BIN\$|tty1::respawn:/sbin/getty 38400 tty1|" "$INITTAB"
        echo "tty1 set back to getty. Reboot to apply."
    fi
    exit 0
fi

echo "=== 1/4 Installing build tools ==="
apk add build-base
# Debugging tools (valgrind lives in the community repository on some Alpine versions)
apk add gdb valgrind || echo "Note: could not install gdb/valgrind (enable the community repo in /etc/apk/repositories)."

echo "=== 2/4 Building the game ==="
make clean
make
make test

echo "=== 3/4 Installing $BIN ==="
install -m 755 dots_and_boxes "$BIN"

echo "=== 4/4 Starting the game on tty1 instead of the login prompt ==="
[ -f "$BACKUP" ] || cp "$INITTAB" "$BACKUP"
if grep -q "^tty1::" "$INITTAB"; then
    sed -i "s|^tty1::.*|tty1::respawn:$BIN|" "$INITTAB"
else
    echo "tty1::respawn:$BIN" >> "$INITTAB"
fi
grep "^tty1::" "$INITTAB"

echo
echo "Done. Type 'reboot': the VM will open directly into the game."
echo "(A normal login is still available on tty2 with Alt+F2, for maintenance.)"
