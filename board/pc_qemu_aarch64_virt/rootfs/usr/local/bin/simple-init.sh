#!/bin/sh

[ -f /root/firstboot.sh ] && /root/firstboot.sh

echo 3 > /proc/sys/kernel/printk