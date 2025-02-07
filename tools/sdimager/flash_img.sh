#!/usr/bin/env bash

IMAGE_FILE="$1"
SD_DEVICE="$2"

if [[ -z "$IMAGE_FILE" || -z "$SD_DEVICE" ]]; then
    echo "Usage: $0 <image file> <SD device>"
    exit 1
fi

if [[ ! -f "$IMAGE_FILE" ]]; then
    echo "Image file not found: $IMAGE_FILE"
    exit 1
fi

# Make sure device size is  around 8gb to prevent people from accidentally overwriting their hard drive

DEVICE_SIZE=$(blockdev --getsize64 "$SD_DEVICE")
if (( DEVICE_SIZE <= 7400000000 || DEVICE_SIZE >= 8000000000 )); then
    echo "Device size is not 8GB: $DEVICE_SIZE"
    echo "Are you sure you want to continue? (y/n)"
    read -r response
    if [[ "$response" != "y" ]]; then
        exit 1
    fi
fi

# Copy the image to the SD card
dd if="$IMAGE_FILE" of="$SD_DEVICE"
sync