#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

amixer -c 1 set "Master" "80%"

/usr/local/bin/led-control.sh &
/usr/local/bin/freqfunctions.sh powersave
rfkill block bluetooth

killall python3
export PYTHONUNBUFFERED=1
nohup /usr/bin/python3 /usr/local/bin/simple-keymon.py &
unset PYTHONUNBUFFERED

printf "\033c" > /dev/tty3
printf "\033c" > /dev/tty4

while [ ! -e /dev/input/by-path/platform-rocknix-singleadc-joypad-event-joystick ]; do
    echo "Waiting joypad device ready..." >/dev/tty1
    sleep 0.5
done
cd /usr/local/bin && /usr/local/bin/simple-launcher &

chvt 3
sleep infinity