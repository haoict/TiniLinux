#!/bin/bash

export SDL_GAMECONTROLLERCONFIG_FILE="/root/gamecontrollerdb.txt"

if [[ ! -z $(ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}') ]]; then
  ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9
fi
/usr/local/bin/gptokeyb2 -1 "btop" -c "/root/gptokeyb2.ini" >/dev/null 2>&1 &

# btop++ needs an interactive shell to run
openvt -s -w -- /usr/local/bin/btop --force-utf
