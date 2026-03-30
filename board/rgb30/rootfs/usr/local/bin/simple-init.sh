#!/bin/sh

[ -f /root/firstboot.sh ] && /root/firstboot.sh

amixer -c 1 set "Master" "80%"

/usr/local/bin/led-control.sh &
/usr/local/bin/freqfunctions.sh powersave

if [ -f /usr/local/bin/simple-launcher ]; then
    chvt 3;
else
    # consoleonly boards
    systemctl set-default multi-user.target
fi

printf "\033c" > /dev/tty3
printf "\033c" > /dev/tty4

PYTHONUNBUFFERED=1 exec /usr/local/bin/simple-keymon.py
