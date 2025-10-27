#!/bin/bash

export SDL_GAMECONTROLLERCONFIG_FILE="/root/gamecontrollerdb.txt"

if [[ ! -z $(ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}') ]]; then
  ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9
fi
/usr/local/bin/gptokeyb2 -c "/root/gptokeyb2.ini" >/dev/null 2>&1 &

openvt -s -w -- "$@"

ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9
