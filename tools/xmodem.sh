#!/usr/bin/env bash
PORT=/dev/ttyUSB0  # Adjust as needed
LOGFILE=/tmp/uart_log.txt
FILE=$1

# Check if file exists
if [ ! -f "$FILE" ]; then
    echo "Error: File $FILE not found!"
    exit 1
fi

# Make sure we have the required tools
command -v minicom >/dev/null 2>&1 || { echo "Error: minicom not found. Please install it."; exit 1; }

# Set up the serial port
stty -F $PORT 115200 cs8 -parenb -cstopb raw -echo -hupcl

# Create a temporary minicom script
TMPSCRIPT=$(mktemp)
cat > "$TMPSCRIPT" << EOF
send ""
! sleep 1
send ""
! sleep 1
expect "<<<UART_READY>>>"
! sleep 0.5
send XMODEM $FILE
! sleep 1
send ""
! sleep 1
send ""
! sleep 1
send ""
! sleep 1
send ^D
! sleep 1
! echo "Transfer completed"
EOF

# Run minicom with the script
minicom -D $PORT -S "$TMPSCRIPT" -C $LOGFILE

# Clean up
rm "$TMPSCRIPT"
echo "Check $LOGFILE for details"
