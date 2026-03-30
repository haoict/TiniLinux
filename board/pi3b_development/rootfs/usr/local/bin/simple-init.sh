#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

# Disable console blanking
echo -ne "\033[9;0]" > /dev/tty1

/usr/local/bin/freqfunctions.sh balanced_performance
