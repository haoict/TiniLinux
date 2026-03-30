#!/bin/sh

[ -f /root/firstboot.sh ] && /root/firstboot.sh

amixer -c 0 set "PCM" "100%" # aplay -l && aplay /usr/share/sounds/test.wav

/usr/local/bin/freqfunctions.sh powersave

printf "\033c" > /dev/tty3
printf "\033c" > /dev/tty4

PYTHONUNBUFFERED=1 exec /usr/local/bin/simple-keymon.py
