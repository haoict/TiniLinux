#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
    systemctl set-default graphical.target
    sleep 2
    systemctl enable sway.service --now
    sleep 2
    systemctl enable launcher.service --now
fi

echo 3 > /proc/sys/kernel/printk

# Disable console blanking
echo -ne "\033[9;0]" > /dev/tty1

amixer -c 0 set "DAC" "100%"
amixer -c 0 set "Line Out" "80%"

/usr/local/bin/freqfunctions.sh powersave
rfkill block bluetooth

killall python3
PYTHONUNBUFFERED=1 /usr/bin/python3 /usr/local/bin/simple-keymon.py
