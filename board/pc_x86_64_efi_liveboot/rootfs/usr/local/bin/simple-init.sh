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

# cd /usr/local/bin && /usr/local/bin/simple-launcher &

sleep infinity