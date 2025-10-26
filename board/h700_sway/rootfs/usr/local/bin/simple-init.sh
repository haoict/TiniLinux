#!/bin/sh

if [ -f /root/firstboot.sh ]; then
    /root/firstboot.sh
fi

echo 3 > /proc/sys/kernel/printk

# Disable console blanking
echo -ne "\033[9;0]" > /dev/tty1

amixer -c 0 set "DAC" "100%"
amixer -c 0 set "Line Out" "80%"

#killall weston
killall sway
killall simple-launcher

mkdir -p /run/user/0
export XDG_RUNTIME_DIR=/run/user/0
#weston --backend=drm-backend.so --shell=kiosk-shell.so &
sway -c /root/.config/sway/config &
sleep 3

/usr/local/bin/freqfunctions.sh ondemand

killall python3
export PYTHONUNBUFFERED=1
nohup /usr/bin/python3 /usr/local/bin/simple-keymon.py &
unset PYTHONUNBUFFERED

export SDL_VIDEODRIVER=wayland
export WAYLAND_DISPLAY=$(find /run/user/$UID -regex '.*/wayland-[0-9]*')
/usr/local/bin/simple-launcher &

sleep infinity