#!/usr/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
    echo "Image must be made as root (requires ability to create loopback devices)"
    exit 1
fi

OUT_IMG="$1"
IN_IMG="$2"
MOUNT_POINT="/mnt/osfiles/"
USER=$(logname)
BLOCK_SIZE=512
PART_START=2048

cp "$IN_IMG" "$OUT_IMG"

PART_LOOP=$(losetup -f)
losetup "$PART_LOOP" "$OUT_IMG" -o "$((PART_START * BLOCK_SIZE))"

mkdir -p "$MOUNT_POINT"
mount "$PART_LOOP" "$MOUNT_POINT"

cp -rv "$3"/* "$MOUNT_POINT"

umount "$MOUNT_POINT"
losetup -d "$PART_LOOP"

chown "$USER":"$USER" "$OUT_IMG"
