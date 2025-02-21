#!/usr/bin/env bash
set -eo pipefail

PORT=/dev/ttyUSB0
BAUDRATE=115200
LOGFILE="/tmp/uart_full.log"
IMAGE_FILE="$1"

# Cleanup function
cleanup() {
    kill $(jobs -p) 2>/dev/null || true
    rm -f /tmp/uart_{in,out,pts}
    stty sane
}
trap cleanup EXIT

# Create FIFOs first
mkfifo /tmp/uart_in /tmp/uart_out

# Start socat with full duplex logging
socat -d -d -lf "$LOGFILE" \
    PTY,link=/tmp/uart_pts,rawer,wait-slave \
    SYSTEM:"
        stty -F $PORT $BAUDRATE cs8 -parenb -cstopb raw -echo;
        tee -a $LOGFILE < $PORT > /tmp/uart_in &
        cat /tmp/uart_out > $PORT
    " &

# Wait for PTY to be ready
while [ ! -e /tmp/uart_pts ]; do sleep 0.1; done

# Start live monitor in background
echo -e "\n--- LIVE OUTPUT ---"
cat /tmp/uart_in | tee /dev/stderr &

# XMODEM Transfer
echo "Waiting for <<<UART_READY>>>..."
timeout 30 grep -q -m1 "<<<UART_READY>>>" <(tail -f $LOGFILE)

echo "Starting XMODEM transfer..."
sx -vv --xmodem --crc "$IMAGE_FILE" < /tmp/uart_pts > /tmp/uart_pts

# Wait for completion
echo "Waiting for <<<UART_DONE>>>..."
timeout 30 grep -q -m1 "<<<UART_DONE>>>" <(tail -f $LOGFILE)

echo -e "\nTransfer complete! Log: $LOGFILE"
