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
command -v sx >/dev/null 2>&1 || { echo "Error: sx command not found. Please install lrzsz package."; exit 1; }

# Set up the serial port with proper settings
#stty -F $PORT 115200 cs8 -parenb -cstopb raw -echo
#dd if=$PORT iflag=nonblock count=0 2>/dev/null

# Start monitoring the port for the ready signal
exec 3<>$PORT
cat <&3 | tee $LOGFILE | while read -r line; do
    if [[ "$line" == *"<<<UART_READY>>>"* ]]; then
        echo "Detected ready signal. Starting XMODEM transfer..."
        # Give a small delay for the receiver to be fully ready
        sleep 0.5
        # Close the read pipe to allow sx to take over
        pkill -P $$ cat
        # Start the transfer
        sx -vv --xmodem --crc $FILE >&3 <&3
        RESULT=$?
        echo "Transfer completed with exit code $RESULT"

        sleep 0.5

        # Now continue to show the output
        echo "Showing serial output. Press Ctrl+C to exit."
        # Close our file descriptor and reopen to ensure clean state
        exec 3>&-
        cat $PORT
    fi
done
