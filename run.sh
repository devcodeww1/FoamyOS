#!/bin/sh
set -e
cd "$(dirname "$0")"

if [ -e /dev/kvm ]; then
    KVM_ARGS="-enable-kvm -cpu host"
else
    KVM_ARGS="-cpu qemu64"
fi

if [ ! -f foamyos-persist.img ]; then
    echo "Creating 64 MB persistent disk (foamyos-persist.img)..."
    truncate -s 64M foamyos-persist.img
fi

exec qemu-system-x86_64 \
    $KVM_ARGS \
    -m 1024 \
    -smp 2 \
    -name FoamyOS \
    -kernel boot/bzImage \
    -initrd boot/initramfs.cpio.gz \
    -append "console=tty0 rdinit=/init" \
    -drive file=foamyos-persist.img,format=raw,if=ide \
    -nic user,model=virtio-net-pci \
    -vga std \
    -display gtk \
    -device qemu-xhci \
    -device usb-tablet
