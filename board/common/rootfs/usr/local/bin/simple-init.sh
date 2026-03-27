#!/bin/sh

# This file is meant to be overwritten by specific board simple-init.sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi
