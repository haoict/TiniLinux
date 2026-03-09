#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

amixer -c 0 set "PCM" "100%" # aplay -l && aplay /usr/share/sounds/test.wav

/usr/local/bin/freqfunctions.sh powersave

killall python3
export PYTHONUNBUFFERED=1
nohup /usr/bin/python3 /usr/local/bin/simple-keymon.py &
unset PYTHONUNBUFFERED

printf "\033c" > /dev/tty3
printf "\033c" > /dev/tty4

while [ ! -e /dev/dri/renderD128 ]; do
    echo "Waiting GPU device ready..." >/dev/tty1
    sleep 0.5
done

/usr/local/bin/simple-launcher -width 400 -height 400 -scale 0.6 -itemsPerPage 6 -font /usr/share/fonts/Ubuntu.ttf &

chvt 3
sleep infinity