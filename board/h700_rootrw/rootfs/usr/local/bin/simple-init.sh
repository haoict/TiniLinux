#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

amixer -c 0 set "DAC" "100%"
amixer -c 0 set "Line Out" "80%"

/usr/local/bin/freqfunctions.sh powersave
rfkill block bluetooth

killall python3
export PYTHONUNBUFFERED=1
nohup /usr/bin/python3 /usr/local/bin/simple-keymon.py &
unset PYTHONUNBUFFERED

while [ ! -e /dev/input/by-path/platform-rocknix-singleadc-joypad-event-joystick ]; do
    echo "Waiting joypad device ready..." > /dev/tty1
    sleep 0.5
done
cd /usr/local/bin && /usr/local/bin/simple-launcher &

chvt 3
sleep infinity