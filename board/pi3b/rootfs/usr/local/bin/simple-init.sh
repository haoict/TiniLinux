#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
    i=0; while [ $i -lt 10 ] && [ ! -d /roms/simple-launcher ]; do echo "Waiting /roms ready..."; sleep 0.5; i=$((i+1)); done;
    cp /usr/share/fonts/Ubuntu.ttf /roms/simple-launcher/font.ttf
fi

amixer -c 0 set "PCM" "100%" # aplay -l && aplay /usr/share/sounds/test.wav

/usr/local/bin/freqfunctions.sh powersave

if [ -f /usr/local/bin/simple-launcher ]; then
    printf "\033c" > /dev/tty3
    printf "\033c" > /dev/tty4
else
    # consoleonly boards
    systemctl set-default multi-user.target
fi

PYTHONUNBUFFERED=1 exec /usr/local/bin/simple-keymon.py
