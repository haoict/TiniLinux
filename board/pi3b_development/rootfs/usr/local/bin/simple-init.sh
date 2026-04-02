#!/bin/sh

[ -f /root/firstboot.sh ] && /root/firstboot.sh

/usr/local/bin/freqfunctions.sh balanced_performance

PYTHONUNBUFFERED=1 exec /usr/local/bin/simple-keymon.py
