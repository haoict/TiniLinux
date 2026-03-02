#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

# Disable console blanking
echo -ne "\033[9;0]" > /dev/tty1

amixer -c 0 set "PCM" "100%" # aplay -l && aplay /usr/share/sounds/test.wav

# /usr/local/bin/freqfunctions.sh powersave

# cd /usr/local/bin && /usr/local/bin/simple-launcher 480 480 0.5 &

sleep infinity