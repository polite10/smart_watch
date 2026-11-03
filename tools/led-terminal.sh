#!/bin/sh
# Open PuTTY on the board's ST-LINK serial port (see docs/led-uart-commands.md).
# Usage: tools/led-terminal.sh [port]    default: first /dev/cu.usbmodem*
port="${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}"
if [ -z "$port" ]; then
    echo "ST-LINK seri portu bulunamadı. Kartı CN5 USB portundan bağlayın." >&2
    exit 1
fi
mkdir -p "$HOME/.putty/sessions"
# PuTTY ignores -serial after -load, so the port goes into the session itself.
# LocalEcho 0 = Force on: the firmware does not echo. LocalEdit 1 = Force off: keys go at once,
# so Ctrl+L works without Enter; Backspace is unavailable. CRImpliesLF keeps replies off the typed line.
cat > "$HOME/.putty/sessions/STM32-LED" <<EOF
Protocol=serial
SerialLine=$port
SerialSpeed=115200
SerialDataBits=8
SerialStopHalfbits=2
SerialParity=0
SerialFlowControl=0
LocalEcho=0
LocalEdit=1
CRImpliesLF=1
EOF
exec putty -load STM32-LED
