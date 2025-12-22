#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

# Disable console blanking
echo -ne "\033[9;0]" > /dev/tty1

sleep infinity