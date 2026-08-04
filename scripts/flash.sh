#!/usr/bin/env bash
#
# Flash both KLOR halves from a downloaded firmware.zip extract.
# Usage:
#   ./scripts/flash.sh --firmware-folder ~/Downloads/main-08022026-164041-klor-firmware [--reset]

set -euo pipefail

VOLUME="/Volumes/NICENANO"
LEFT_FILE="klor_left-nice_nano_v2-zmk.uf2"
RIGHT_FILE="klor_right-nice_nano_v2-zmk.uf2"
RESET_FILE="settings_reset-nice_nano_v2-zmk.uf2"

FIRMWARE_FOLDER=""
DO_RESET=0

while [ $# -gt 0 ]; do
    case "$1" in
        --firmware-folder)
            FIRMWARE_FOLDER="$2"
            shift 2
            ;;
        --firmware-folder=*)
            FIRMWARE_FOLDER="${1#*=}"
            shift
            ;;
        --reset)
            DO_RESET=1
            shift
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [ -z "$FIRMWARE_FOLDER" ]; then
    echo "Error: --firmware-folder is required." >&2
    exit 1
fi

if [ ! -d "$FIRMWARE_FOLDER" ]; then
    echo "Error: firmware folder not found: $FIRMWARE_FOLDER" >&2
    exit 1
fi

LEFT_PATH="$FIRMWARE_FOLDER/$LEFT_FILE"
RIGHT_PATH="$FIRMWARE_FOLDER/$RIGHT_FILE"
RESET_PATH="$FIRMWARE_FOLDER/$RESET_FILE"

if [ ! -f "$LEFT_PATH" ]; then
    echo "Error: left firmware not found: $LEFT_PATH" >&2
    exit 1
fi

if [ ! -f "$RIGHT_PATH" ]; then
    echo "Error: right firmware not found: $RIGHT_PATH" >&2
    exit 1
fi

if [ "$DO_RESET" -eq 1 ] && [ ! -f "$RESET_PATH" ]; then
    echo "Error: reset firmware not found: $RESET_PATH" >&2
    exit 1
fi

wait_for_volume() {
    while [ ! -d "$VOLUME" ]; do
        sleep 0.2
    done
}

wait_for_unmount() {
    while [ -d "$VOLUME" ]; do
        sleep 0.2
    done
}

# flash <Label> <file> <wait-message>
flash() {
    local label="$1" file="$2" message="$3"
    echo "$message"
    wait_for_volume
    cp "$file" "$VOLUME"
    sync
    wait_for_unmount
    echo "${label} half flashed successfully."
}

if [ "$DO_RESET" -eq 1 ]; then
    flash "Left" "$RESET_PATH" "Waiting for left half to enter bootloader mode to reset..."
fi
flash "Left" "$LEFT_PATH" "Waiting for left half to enter bootloader mode..."

if [ "$DO_RESET" -eq 1 ]; then
    flash "Right" "$RESET_PATH" "Waiting for right half to enter bootloader mode to reset..."
fi
flash "Right" "$RIGHT_PATH" "Waiting for right half to enter bootloader mode..."

echo "Both halves flashed successfully."
