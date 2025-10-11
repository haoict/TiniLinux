#!/bin/bash
echo "Please enter wifi password:"
read user_input

echo "$user_input" > /tmp/wifi-password.txt
ps aux | grep SimpleTerminal | grep -v grep | awk '{print $1}' | xargs kill -9
exit 0
