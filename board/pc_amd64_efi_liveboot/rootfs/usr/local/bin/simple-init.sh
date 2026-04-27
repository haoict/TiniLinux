#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
    systemctl set-default graphical.target
    sleep 2
    systemctl enable sway.service --now
    sleep 2
    systemctl enable launcher.service --now
fi

# cd /usr/local/bin && /usr/local/bin/simple-launcher &

sleep infinity