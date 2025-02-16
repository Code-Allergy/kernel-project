#!/usr/bin/env bash

PORT=/dev/ttyUSB0  # Adjust as needed
LOGFILE=/tmp/uart_log.txt
FILE=$1

cat $PORT | tee $LOGFILE | while read -r line; do
    if [[ "$line" == *"<<<UART_READY>>>"* ]]; then
        echo "Triggering XMODEM Transfer..."
        sx -X $1 > $PORT < $PORT
    fi
done
