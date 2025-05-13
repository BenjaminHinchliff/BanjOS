#!/usr/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Image must be made as root (requires ability to create loopback devices)"
  exit 1
fi

IMG="$1"
MOUNT_POINT="/mnt/osfiles/"
USER=$(logname)
BLOCK_SIZE=512
DISK_SIZE=32768 # disk size, in pages/sectors
PART_START=2048

dd if=/dev/zero of="$IMG" bs=512 count="$DISK_SIZE"
parted "$IMG" mklabel msdos
parted "$IMG" mkpart primary ext2 "$PART_START"s "$((DISK_SIZE - PART_START))"s
parted "$IMG" set 1 boot on

DEV_LOOP=$(losetup -f)
losetup "$DEV_LOOP" "$IMG"

PART_LOOP=$(losetup -f)
losetup "$PART_LOOP" "$IMG" -o "$((PART_START * BLOCK_SIZE))"

mkfs.ext2 "$PART_LOOP"

mkdir -p "$MOUNT_POINT"
mount "$PART_LOOP" "$MOUNT_POINT"

grub-install --root-directory="$MOUNT_POINT" --target=i386-pc --no-floppy --modules="normal part_msdos ext2 multiboot multiboot2" "$DEV_LOOP"

umount "$MOUNT_POINT"
losetup -d "$PART_LOOP"
losetup -d "$DEV_LOOP"

chown "$USER":"$USER" "$IMG"
