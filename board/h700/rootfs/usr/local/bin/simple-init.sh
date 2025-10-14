#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

# Disable console blanking
echo -ne "\033[9;0]" > /dev/tty1

/usr/local/bin/freqfunctions.sh powersave

killall python3
nohup /usr/bin/python3 /usr/local/bin/simple-keymon.py &

cd /usr/local/bin && /usr/local/bin/simple-launcher &

sleep infinity