#!/bin/bash


# Configuration
MAKE="/usr/bin/make"  # Verify this path actually exists!
ESCAPE_SEQUENCE=$'\x1b[signal]'
RESPONSE_MESSAGE="Response from host"
OUTPUT_FIFO=$(mktemp -u)  # Create unique FIFO name

# Create named pipe
mkfifo "$OUTPUT_FIFO" || { echo "Failed to create FIFO"; exit 1; }

cleanup() {
    kill -9 "$QEMU_PID" 2>/dev/null
    rm -f "$OUTPUT_FIFO"
    exit
}
trap cleanup SIGINT SIGTERM EXIT

# Start command with proper argument handling
if ! /usr/bin/unbuffer echo hello > "$OUTPUT_FIFO" 2>&1 & then
    echo "Failed to start command"
    cleanup
fi
QEMU_PID=$!

echo "Monitoring output..."

# Read from FIFO with timeout
while read -r -t 5 LINE || [[ $? -eq 142 ]]; do
    # Display output
    echo "$LINE"
    
    # Check for escape sequence
    if [[ "$LINE" == *"$ESCAPE_SEQUENCE"* ]]; then
        echo "$RESPONSE_MESSAGE"
        echo "Detected escape sequence - response sent"
    fi
done < "$OUTPUT_FIFO"

cleanup